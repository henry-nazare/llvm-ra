#define DEBUG_TYPE "ra-graph"

#include "RAGraphBase.h"

#include "SAGE/SAGEExprGraph.h"
#include "SAGE/SAGENameVault.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Module.h"

using namespace llvm;

struct SRAGraphObjInfo : public PythonObjInfo {
  SRAGraphObjInfo(const char *Fn)
      : PythonObjInfo("llvmra.graph", "RAGraph", Fn) {}
};

static SRAGraphObjInfo graph_SRAGraph(nullptr);
RAGraphBase::RAGraphBase(
    const Module *M, const CallGraph *CG, const DataLayout *DL,
    const SAGEExprGraph *SEG, SAGENameVault *SNV)
        : SAGEAnalysisGraph(graph_SRAGraph({}), *SNV), M(M), CG(CG), DL(DL),
          SEG(SEG), SNV(SNV) {
}

