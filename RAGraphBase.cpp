#define DEBUG_TYPE "ra-graph"

#include "RAGraphBase.h"

#include "SAGE/SAGEExpr.h"
#include "SAGE/SAGEExprGraph.h"
#include "SAGE/SAGENameVault.h"

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"

#include <functional>
#include <stack>
#include <unordered_map>

using namespace llvm;
using llvmpy::Get;

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
    [](const Function *F, SAGEExprGraph *SEG) -> const SAGEExpr {
  return SEG->getExpr(&*F->arg_begin());
};

typedef
    std::function<bool (const FunctionType*, LLVMContext&)>
    FunctionVerifyFn;
typedef
    std::function<const SAGEExpr (const Function*, SAGEExprGraph*)>
    FunctionGetSizeFn;
typedef std::pair<FunctionVerifyFn, FunctionGetSizeFn> FunctionFnPair;

std::unordered_map<std::string, FunctionFnPair> AllocationFns = {
  {"malloc", std::make_pair(IsMallocLike, GetMallocLikeSize)},
};

SAGEExpr RAGraphBase::getSize(const Value *V) {
  static PythonAttrInfo graph_Node_state("state");
  return graph_Node_state.get(getNode(V));
}

unsigned RAGraphBase::getMemId(const Value *V) {
  static PythonAttrInfo attr_state("state");
  static PythonAttrInfo attr_memid("memid");
  return PyInt_AsLong(attr_memid.get(attr_state.get(getNode(V))));
}

PyObject *RAGraphBase::getNode(const Value *V) {
  assert((V->getType()->isPointerTy() || isa<ReturnInst>(V))
      && "Value is not a pointer");

  auto It = Node_.find(V);
  if (It != Node_.end()) {
    return It->second;
  }

  auto Gen = getGenerator(getNodeName(V), SAGEExpr::GetMinusInf().get());
  setNode(V, Gen);
  return Gen;
}

PyObject *RAGraphBase::getNodeName(const Value *V) const {
  return Get(SNV->getName(V));
}

void RAGraphBase::initialize() {
  ReversePostOrderTraversal<const CallGraph*> RPOT(CG);
  std::stack<const Function*> Stack;
  for (auto &N : RPOT) {
    if (const Function *F = N->getFunction()) {
      Stack.push(F);
    }
  }

  while (!Stack.empty()) {
    auto F = Stack.top();
    Stack.pop();
    initializeFunction(F);
  }
}

void RAGraphBase::initializeFunction(const Function *F) {
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
    return (void) setNode(F, getNoAliasGenerator(getNodeName(F), Size.get()));
  }

  if (F->isIntrinsic() || F->isDeclaration()) {
    if (F->getFunctionType()->getReturnType()->isPointerTy()) {
      setNode(F, getGenerator(getNodeName(F), SAGEExpr::GetMinusInf().get()));
    }
    return;
  }

  initializePtrInsts(F);
}

void RAGraphBase::initializePtrInsts(const Function *F) {
  Incoming.clear();

  ReversePostOrderTraversal<const Function*> RPOT(F);
  for (auto &BB : RPOT) {
    for (auto &I : *BB) {
      if (I.getType()->isPointerTy() || isa<ReturnInst>(&I)) {
        addPtrInst(F, &I);
      }
    }
  }

  for (auto &P : Incoming) {
    for (auto &V : P.second) {
      addIncoming(V, P.first);
    }
  }
}

void RAGraphBase::addPtrInst(const Function *F, const Instruction *I) {
  if (auto CI = dyn_cast<const CallInst>(I)) {
    if (CI->getType()->isPointerTy()) {
      return (void) addCallInst(CI);
    }
  }

  if (auto AI = dyn_cast<const AllocaInst>(I)) {
    if (AI->isStaticAlloca()) {
      return (void) addAllocaInst(AI);
    }
  }

  if (auto CI = dyn_cast<const CastInst>(I)) {
    setNode(CI, getId(getNodeName(CI)));
  }

  if (auto GEP = dyn_cast<const GetElementPtrInst>(I)) {
    if (GEP->getNumIndices() == 1) {
      return (void) addGEPInst(GEP);
    }
  }

  if (F->getFunctionType()->getReturnType()->isPointerTy()) {
    if (auto RI = dyn_cast<const ReturnInst>(I)) {
      return (void) addReturnInst(RI, F);
    }
  }

  if (isa<const BitCastInst>(I)) {
    return (void) addIncoming(I->getOperand(0), I);
  }

  setNode(I, getGenerator(getNodeName(I), SAGEExpr::GetMinusInf().get()));
}

void RAGraphBase::addAllocaInst(const AllocaInst *AI) {
  auto TypeSize = DL->getTypeSizeInBits(AI->getType()->getContainedType(0))/8;
  auto Size = SAGEExpr(TypeSize);
  auto Gen = getGenerator(getNodeName(AI), Size.get());
  setNode(AI, Gen);
}

void RAGraphBase::addCallInst(const CallInst *CI) {
  Function *F = CI->getCalledFunction();
  if (!F) {
    return;
  }

  std::map<PyObject*, PyObject*> Dict;
  unsigned Idx = 0;
  for (auto &A : F->args()) {
    if (A.getType()->isIntegerTy()) {
      Dict[SEG->getExpr(&A).get()] = SEG->getExpr(CI->getArgOperand(Idx)).get();
    }
    ++Idx;
  }

  setNode(CI, getReplacer(getNodeName(CI), Dict));
  Incoming[CI] = {F};
}

void RAGraphBase::addGEPInst(const GetElementPtrInst *GEP) {
  auto Index = SEG->getExpr(GEP->getOperand(1));
  auto TypeSize = DL->getTypeSizeInBits(GEP->getType()->getContainedType(0))/8;
  auto Size = SAGEExpr(TypeSize);
  setNode(GEP, getIndex(getNodeName(GEP), Index.get(), Size.get()));
  addIncoming(GEP->getPointerOperand(), GEP);
}

void RAGraphBase::addReturnInst(const ReturnInst *RI, const Function *F) {
  setNode(RI, getId(getNodeName(RI)));
  addIncoming(RI->getOperand(0), RI);
  setNode(F, getId(getNodeName(F)));
  addIncoming(RI, F);
}

PyObject *RAGraphBase::getGenerator(PyObject *Obj, PyObject *Expr) const {
  static SRAGraphObjInfo graph_SRAGraph_get_generator("get_generator");
  return graph_SRAGraph_get_generator({get(), Obj, Expr});
}

PyObject *RAGraphBase::getNoAliasGenerator(PyObject *Obj, PyObject *Expr) const {
  static SRAGraphObjInfo graph_SRAGraph_get_generator("get_noalias_generator");
  return graph_SRAGraph_get_generator({get(), Obj, Expr});
}

PyObject *RAGraphBase::getId(PyObject *Obj) const {
  static SRAGraphObjInfo graph_SRAGraph_get_generator("get_id");
  return graph_SRAGraph_get_generator({get(), Obj});
}

PyObject *RAGraphBase::getReplacer(
    PyObject *Obj, std::map<PyObject*, PyObject*> Dict) const {
  static SRAGraphObjInfo graph_SRAGraph_get_replacer("get_replacer");
  return graph_SRAGraph_get_replacer({get(), Obj, llvmpy::MakeDict(Dict)});
}

PyObject *RAGraphBase::
    getIndex(PyObject *Obj, PyObject *Expr, PyObject *Size) const {
  static SRAGraphObjInfo graph_SRAGraph_get_index("get_indexation");
  return graph_SRAGraph_get_index({get(), Obj, Expr, Size});
}

