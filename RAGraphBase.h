#ifndef _RAGRAPHBASE_H_
#define _RAGRAPHBASE_H_

// Should always be the first include.
#include "SAGE/Python/PythonInterface.h"

#include "SAGE/SAGEAnalysisGraph.h"

namespace llvm {

class Module;
class CallGraph;
class DataLayout;

} // end namespace llvm

class SAGEExprGraph;
class SAGENameVault;

class RAGraphBase : public SAGEAnalysisGraph {
public:
  RAGraphBase(
      const Module *M, const CallGraph *CG, const DataLayout *DL,
      SAGEExprGraph *SEG, SAGENameVault *SNV);

  void initialize();

private:
  void initializeFunction(Function *F);

  PyObject *getGenerator(PyObject *Obj, PyObject *Expr) const;

  const Module *M;
  const CallGraph *CG;
  const DataLayout *DL;
  SAGEExprGraph *SEG;
  SAGENameVault *SNV;
};

#endif

