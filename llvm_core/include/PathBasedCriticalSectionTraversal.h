#ifndef LLVM_CORE_PATHBASEDCRITICALSECTIONTRAVERSAL_H
#define LLVM_CORE_PATHBASEDCRITICALSECTIONTRAVERSAL_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace hepf {

struct PathBasedCriticalSectionTraversalPass
    : public llvm::PassInfoMixin<PathBasedCriticalSectionTraversalPass> {

private:
  // 1. Declare members to hold the configuration values
  const size_t MaxPaths;
  const size_t MaxLoopIterations;

public:
  // 2. Constructor: Use the struct name and accept the parameters.
  //    The 'explicit' keyword is fine here.
  explicit PathBasedCriticalSectionTraversalPass(
      size_t maxPaths,
      size_t maxLoopIterations);
  PathBasedCriticalSectionTraversalPass() : MaxPaths(100), MaxLoopIterations(100) {};

  // The run method uses the configuration data stored in the struct members.
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
};

} // namespace hepf

#endif // LLVM_CORE_PATHBASEDCRITICALSECTIONTRAVERSAL_H
