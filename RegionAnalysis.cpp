//===------------------------- RegionAnalysis.cpp -------------------------===//
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

  return false;
}

void RegionAnalysis::print(raw_ostream &OS, const Module*) const {
  OS << *G << "\n";
}

