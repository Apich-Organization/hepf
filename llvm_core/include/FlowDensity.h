#ifndef LLVM_CORE_FLOWDENSITY_H
#define LLVM_CORE_FLOWDENSITY_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include <map>

namespace hepf {

// Struct FlowDensity
struct FlowDensity : public llvm::PassInfoMixin<FlowDensity> {

  // 1. CRITICAL FIX: Changed the key type to const Function*
  std::map<const llvm::Function *, float> entropyMap;

  // 2. Minor improvement: Moved mutable state to local variables in run()
  // unless you truly intend to accumulate results across multiple runs.
  // If run() is the only place these are used, they can be removed from here.
  // float totalGradient = 0; // Removing this line
  // int edgeCount = 0;       // Removing this line

  void calculateEntropy(llvm::Module &M);

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

} // namespace hepf

#endif // LLVM_CORE_FLOWDENSITY_H
