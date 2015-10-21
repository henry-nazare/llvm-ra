#ifndef _RAGRAPHBASE_H_
#define _RAGRAPHBASE_H_

// Should always be the first include.
#include "SAGE/Python/PythonInterface.h"

#include "SAGE/SAGEAnalysisGraph.h"

#include "llvm/ADT/DenseMap.h"

#include <list>
#include <map>

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

  SAGEExpr getSize(const Value *V);
  unsigned getMemId(const Value *V);

private:
  PyObject *getNode(const Value *V) override;
  PyObject *getNodeName(const Value *V) const override;

  void initializeFunction(const Function *F);
  void initializePtrInsts(const Function *F);

  void addPtrInst(const Function *F, const Instruction *I);
  void addAllocaInst(const AllocaInst *AI);
  void addCallInst(const CallInst *CI);
  void addReturnInst(const ReturnInst *RI, const Function *F);
  void addGEPInst(const GetElementPtrInst *GEP);

  PyObject *getGenerator(PyObject *Obj, PyObject *Expr) const;
  PyObject *getNoAliasGenerator(PyObject *Obj, PyObject *Expr) const;
  PyObject *getId(PyObject *Obj) const;
  PyObject
      *getReplacer(PyObject *Obj, std::map<PyObject*, PyObject*> Dict) const;
  PyObject *getIndex(PyObject *Obj, PyObject *Expr, PyObject *Size) const;

  const Module *M;
  const CallGraph *CG;
  const DataLayout *DL;
  SAGEExprGraph *SEG;
  SAGENameVault *SNV;

  DenseMap<const Instruction*, std::list<const Value*>> Incoming;
};

#endif

