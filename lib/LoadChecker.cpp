//=============================================================================
// FILE:
//   LoadChecker.cpp
//
// DESCRIPTION:
//   Checks if load instructions dereference a nullptr. If they do, this 
//   plugin prints an error message (using debug info if available) and exits 
//   the process early.
//
// USAGE:
//   $ opt -load-pass-plugin <BUILD_DIR>/lib/libLoadChecker.so \
//     -passes="load-checker" <bitcode-file> -o instrumented.bin
//   $ lli instrumented.bin
//
// LICENSE: MIT
//=============================================================================
#include "LoadChecker.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#define DEBUG_TYPE "load-checker"

bool LoadChecker::runOnModule(llvm::Module& M) {
  bool instrumented = false;

  auto& CTX = M.getContext();

  for(auto& F : M) {
    for(auto& BB : F) {
      for(auto& I : BB) {
        if(auto* loadInst = llvm::dyn_cast<llvm::LoadInst>(&I); loadInst) {
          llvm::Value* addr = loadInst->getOperand(0);
          const auto& debugLoc = loadInst->getDebugLoc();
          // IRBuilder will insert before the loadInst
          llvm::IRBuilder<> builder(loadInst);
          // Compare the adress to the to NULL
          auto* cmp = builder.CreateICmpEQ(addr, llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(addr->getType())));
          // Create a function to check if the addr is NULL and exit early
          std::string funcName = "nullChecker";
          // Debug info will only be available if the `-g` option was passed to clang
          if(debugLoc) {
            funcName += "_";
            funcName += std::to_string(debugLoc.getLine());
            funcName += "_";
            funcName += std::to_string(debugLoc.getCol());
          }
          const bool funcExists = M.getFunction(funcName) != nullptr;
          llvm::FunctionType* nullCheckerTy = llvm::FunctionType::get(llvm::Type::getVoidTy(CTX),
                                                                      {cmp->getType()},
                                                                      /*IsVarArgs=*/false);
          llvm::FunctionCallee nullCheckerC = M.getOrInsertFunction(funcName, nullCheckerTy);
          if(!funcExists) {
            llvm::Function* nullCheckerF = llvm::dyn_cast<llvm::Function>(nullCheckerC.getCallee());
            // Create a basic blocks for nullCheckerF
            llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(CTX, "enter", nullCheckerF);
            llvm::BasicBlock* isNullBlock = llvm::BasicBlock::Create(CTX, "is_null", nullCheckerF);
            llvm::BasicBlock* retBlock = llvm::BasicBlock::Create(CTX, "ret", nullCheckerF);
            llvm::IRBuilder<> funcBuilder(entryBlock);
            funcBuilder.CreateCondBr(nullCheckerF->getArg(0), isNullBlock, retBlock);
            // Inject a declaration of 'printf'
            llvm::FunctionType *printfTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(CTX),
                                                                   llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(CTX)),
                                                                   /*IsVarArgs=*/true);
            llvm::FunctionCallee printfC = M.getOrInsertFunction("printf", printfTy);
            llvm::Function* printfF = llvm::dyn_cast<llvm::Function>(printfC.getCallee());
            printfF->setDoesNotThrow();
            printfF->addParamAttr(0, llvm::Attribute::NoCapture);
            printfF->addParamAttr(0, llvm::Attribute::ReadOnly);
            // Inject a global variable to hold the printf format string
            std::string errorMsg = "Trying to load a NULL pointer";
            if(debugLoc) {
              errorMsg += " at line ";
              errorMsg += std::to_string(debugLoc.getLine());
              errorMsg += ", col ";
              errorMsg += std::to_string(debugLoc.getCol());
            }
            errorMsg += ". Exiting early.\n";
            llvm::Constant* errorMsgStr = llvm::ConstantDataArray::getString(CTX, errorMsg);
            std::string gblVarName = "ErrorMsg";
            if(debugLoc) {
              gblVarName += "_";
              gblVarName += std::to_string(debugLoc.getLine());
              gblVarName += "_";
              gblVarName += std::to_string(debugLoc.getCol());
            }
            llvm::Constant* errorMsgGbl = M.getOrInsertGlobal(gblVarName, errorMsgStr->getType());
            llvm::dyn_cast<llvm::GlobalVariable>(errorMsgGbl)->setInitializer(errorMsgStr);
            // Inject a declaration of 'exit'
            llvm::FunctionType* exitTy = llvm::FunctionType::get(llvm::Type::getVoidTy(CTX),
                                                                 {llvm::Type::getInt32Ty(CTX)},
                                                                 /*IsVarArgs=*/false);
            llvm::FunctionCallee exitC = M.getOrInsertFunction("exit", exitTy);
            // Create contents for isNullBlock and retBlock
            funcBuilder.SetInsertPoint(isNullBlock);
            llvm::Value* errorMsgGblPtr =
              funcBuilder.CreatePointerCast(errorMsgGbl,
                                            llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(CTX)));
            funcBuilder.CreateCall(printfC, {errorMsgGblPtr});
            funcBuilder.CreateCall(exitC, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(CTX), llvm::APInt(32, 1))});
            funcBuilder.CreateBr(retBlock);
            funcBuilder.SetInsertPoint(retBlock);
            funcBuilder.CreateRetVoid();
          }
          // Add function that checks for null
          builder.CreateCall(nullCheckerC, {cmp});
        }
      }
    }
  }

  return instrumented;
}

llvm::PreservedAnalyses LoadChecker::run(llvm::Module& M,
                                         llvm::ModuleAnalysisManager&) {
  bool instrumented = runOnModule(M);

  return instrumented ? llvm::PreservedAnalyses::none()
                      : llvm::PreservedAnalyses::all();
}

llvm::PassPluginLibraryInfo getLoadCheckerPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "load-checker", LLVM_VERSION_STRING,
          [](llvm::PassBuilder& PB) {
            PB.registerPipelineParsingCallback(
              [](llvm::StringRef name, llvm::ModulePassManager& MPM,
                 llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                if(name == "load-checker") {
                  MPM.addPass(LoadChecker());
                  return true;
                }
                return false;
              });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getLoadCheckerPluginInfo();
}
