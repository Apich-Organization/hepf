#pragma once

#include "llvm/IR/PassManager.h"

namespace llvm {
class BranchProbabilityInfo;
class Function;
} // namespace llvm

namespace hepf {

class PathBasedFlowDensityPass
    : public llvm::PassInfoMixin<PathBasedFlowDensityPass> {

private:
    const size_t MaxPaths;
    const size_t MaxLoopIterations;

public:
  explicit PathBasedFlowDensityPass(size_t maxPaths, size_t maxLoopIterations);
  PathBasedFlowDensityPass() : MaxPaths(100), MaxLoopIterations(100) {};

  // Module-level entry point required by the new PM
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);

  // Helper that does the real work on a single function
  // (declared here so we can call it from run())
  void runOnFunction(llvm::Function &F, llvm::BranchProbabilityInfo &BPI);

  // Optional: makes the pass show up in -print-pass-names / opt -passes=
  static bool isRequired() { return true; }
};

} // namespace hepf
