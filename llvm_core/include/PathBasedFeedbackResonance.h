#ifndef LLVM_CORE_PATHBASEDFEEDBACKRESONANCE_H
#define LLVM_CORE_PATHBASEDFEEDBACKRESONANCE_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace hepf {

struct PathBasedFeedbackResonancePass
    : public llvm::PassInfoMixin<PathBasedFeedbackResonancePass> {

private:
  float EntropyThreshold;
  // This private member is initialized by the constructor
  float Threshold;

public:
  // Constructor 1: Takes an optional threshold value.
  explicit PathBasedFeedbackResonancePass(float DefaultEntropyThreshold)
      : EntropyThreshold(DefaultEntropyThreshold) {} // Initialize the member!

  // Constructor 2: Default constructor (usually for registering the pass)
  PathBasedFeedbackResonancePass() = default;

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

  // Add a getter for the threshold (optional, but useful)
  float getThreshold() const { return Threshold; }
};

} // namespace hepf

#endif // LLVM_CORE_PATHBASEDFEEDBACKRESONANCE_H
