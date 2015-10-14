#define DEBUG_TYPE "ra-graph"

#include "RAGraphBase.h"

#include "SAGE/SAGEExprGraph.h"
#include "SAGE/SAGENameVault.h"

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"

#include <functional>
#include <stack>
#include <unordered_map>

using namespace llvm;

struct SRAGraphObjInfo : public PythonObjInfo {
  SRAGraphObjInfo(const char *Fn)
      : PythonObjInfo("llvmra.graph", "RAGraph", Fn) {}
};

static SRAGraphObjInfo graph_SRAGraph(nullptr);
RAGraphBase::RAGraphBase(
    const Module *M, const CallGraph *CG, const DataLayout *DL,
    SAGEExprGraph *SEG, SAGENameVault *SNV)
        : SAGEAnalysisGraph(graph_SRAGraph({}), *SNV), M(M), CG(CG), DL(DL),
          SEG(SEG), SNV(SNV) {
}

static auto IsMallocLike = [](const FunctionType *FT, LLVMContext &Context) {
  return FT->getReturnType() == Type::getInt8PtrTy(Context)
      && FT->getNumParams() == 1
      && (FT->getParamType(0)->isIntegerTy(32)
          || FT->getParamType(0)->isIntegerTy(64));
};

static auto GetMallocLikeSize =
    [](Function *F, SAGEExprGraph *SEG) -> const SAGEExpr {
  return SEG->getExpr(&*F->arg_begin());
};

typedef
    std::function<bool (const FunctionType*, LLVMContext&)>
    FunctionVerifyFn;
typedef
    std::function<const SAGEExpr (Function*, SAGEExprGraph*)>
    FunctionGetSizeFn;
typedef std::pair<FunctionVerifyFn, FunctionGetSizeFn> FunctionFnPair;

std::unordered_map<std::string, FunctionFnPair> AllocationFns = {
  {"malloc", std::make_pair(IsMallocLike, GetMallocLikeSize)},
};

void RAGraphBase::initialize() {
  ReversePostOrderTraversal<const CallGraph*> RPOT(CG);
  std::stack<Function*> Stack;
  for (auto &N : make_range(RPOT.begin(), RPOT.end())) {
    if (Function *F = N->getFunction()) {
      Stack.push(F);
    }
  }
  while (!Stack.empty()) {
    Function *F = Stack.top();
    Stack.pop();
    initializeFunction(F);
  }
}

void RAGraphBase::initializeFunction(Function *F) {
  DEBUG(dbgs() << "RA: initializeFunction: " << F->getName() << "\n");
  if (F->isDeclaration()) {
    auto It = AllocationFns.find(F->getName());
    if (It == AllocationFns.end()) {
      return;
    }

    auto FnPair = It->second;
    if (!FnPair.first(F->getFunctionType(), F->getContext())) {
      return;
    }

    auto Size = FnPair.second(F, SEG);
    return (void) setNode(F, getGenerator(getNodeName(F), Size.get()));
  }

  if (F->isIntrinsic() || F->isDeclaration()) {
    // TODO.
  }
}

PyObject *RAGraphBase::getGenerator(PyObject *Obj, PyObject *Expr) const {
  static SRAGraphObjInfo graph_SRAGraph_get_generator("get_generator");
  return graph_SRAGraph_get_generator({get(), Obj, Expr});
}

