//===------------------------- RegionAnalysis.cpp -------------------------===//
/// This region analysis infers array sizes for pointers throughout the program.
/// For example, in the following:
///
/// int *f(int N) {
///   return malloc(N * sizeof(int)).
/// }
/// int *p = f(M + 1);
///
/// We derive the symbolic size N * 4 as the return value of f and size
/// (M + 1) * 4 as the size of the pointer variable p.
/// When indexing, we simply subract the offset, so &p[1] would have size M * 4.
///
/// The iteration logic is all done in a separate Python module, we simply
/// define the node types in Runtime/llvmra.
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "ra"

#include "RegionAnalysis.h"
#include "RAGraphBase.h"

#include "SAGE/SAGEExprGraph.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Module.h"

using namespace llvm;

static RegisterPass<RegionAnalysis>
    X("region-analysis", "Numerical range analysis with SAGE");
char RegionAnalysis::ID = 0;

void RegionAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<PythonInterface>();
  AU.addRequired<CallGraphWrapperPass>();
  AU.setPreservesAll();
}

bool RegionAnalysis::runOnModule(Module& M) {
  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  DataLayout DL = M.getDataLayout();

  SAGEExprGraph SEG(&M, SNV);
  SEG.initialize();
  SEG.solve();

  G = std::unique_ptr<RAGraphBase>(new RAGraphBase(&M, &CG, &DL, &SEG, &SNV));
  G->initialize();
  G->solve();

  return false;
}

SAGEExpr RegionAnalysis::getSize(Value *V) {
  return G->getSize(V);
}

std::pair<bool, unsigned> RegionAnalysis::getMemId(const Value *V) {
  unsigned Id = G->getMemId(V);
  return std::make_pair(Id != 0, Id);
}

void RegionAnalysis::print(raw_ostream &OS, const Module*) const {
  OS << *G << "\n";
}

