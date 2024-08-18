//=============================================================================
// FILE:
//   LoadChecker.h
// 
// DESCRIPTION:
//   Declares a LoadChecker pass for the new pass manager.
//
// LICENSE: MIT
//=============================================================================

#ifndef LLVM_TUTOR_LOAD_CHECKER_H
#define LLVM_TUTOR_LOAD_CHECKER_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

struct LoadChecker : public llvm::PassInfoMixin<LoadChecker> {
  llvm::PreservedAnalyses run(llvm::Module& M,
                              llvm::ModuleAnalysisManager&);
  bool runOnModule(llvm::Module& M);

  static bool isRequired() { return true; }
};

#endif // LLVM_TUTOR_LOAD_CHECKER_H
