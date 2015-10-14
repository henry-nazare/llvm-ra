#ifndef _REGIONANALYSIS_H_
#define _REGIONANALYSIS_H_

// Should always be the first include.
#include "SAGE/Python/PythonInterface.h"

#include "SAGE/SAGENameVault.h"

#include "llvm/Pass.h"

class RAGraphBase;
class SAGENameVault;
class SAGERange;

class RegionAnalysis : public ModulePass {
public:
  static char ID;
  RegionAnalysis() : ModulePass(ID) { }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module&);

  SAGERange getRange(Value *V);

  virtual void print(raw_ostream &OS, const Module*) const;

private:
  std::unique_ptr<RAGraphBase> G;
  SAGENameVault SNV;
};

#endif

