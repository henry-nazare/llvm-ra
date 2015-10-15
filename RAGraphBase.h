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

class SAGEExpr;
class SAGEExprGraph;
class SAGENameVault;

class RAGraphBase : public SAGEAnalysisGraph {
public:
  RAGraphBase(
      const Module *M, const CallGraph *CG, const DataLayout *DL,
      SAGEExprGraph *SEG, SAGENameVault *SNV);

  void initialize();

  SAGEExpr getSize(Value *V);

private:
  PyObject *getNode(Value *V) override;
  PyObject *getNodeName(Value *V) const override;

  void initializeFunction(Function *F);
  void initializePtrInsts(Function *F);

  void addPtrInst(Function *F, Instruction *I);
  void addAllocaInst(AllocaInst *AI);
  void addCallInst(CallInst *CI);
  void addReturnInst(ReturnInst *RI, Function *F);
  void addGEPInst(GetElementPtrInst *GEP);

  PyObject *getGenerator(PyObject *Obj, PyObject *Expr) const;
  PyObject
      *getReplacer(PyObject *Obj, std::map<PyObject*, PyObject*> Dict) const;
  PyObject *getIndex(PyObject *Obj, PyObject *Expr, PyObject *Size) const;

  const Module *M;
  const CallGraph *CG;
  const DataLayout *DL;
  SAGEExprGraph *SEG;
  SAGENameVault *SNV;

  std::map<Instruction*, std::vector<Value*>> Incoming;
};

#endif

