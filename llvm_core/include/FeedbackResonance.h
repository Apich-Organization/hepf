#ifndef LLVM_CORE_FEEDBACKRESONANCE_H
#define LLVM_CORE_FEEDBACKRESONANCE_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include <map>

namespace hepf {

// Struct FeedbackResonance
struct FeedbackResonance : public llvm::PassInfoMixin<FeedbackResonance> {
  std::map<const llvm::Function *, float> entropyMap;
  const float entropyThreshold = 10.0;
  int cyclicDependencies = 0;

  void calculateEntropy(llvm::Module &M);
  bool isHighEntropy(const llvm::Function *F);

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

} // namespace hepf

#endif // LLVM_CORE_FEEDBACKRESONANCE_H
