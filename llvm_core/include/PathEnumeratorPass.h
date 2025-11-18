#ifndef LLVM_CORE_PATHENUMERATORPASS_H
#define LLVM_CORE_PATHENUMERATORPASS_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace hepf {

struct PathEnumeratorPass : public llvm::PassInfoMixin<PathEnumeratorPass> {
  // 1. Store parameters as members if needed later in 'run'
  size_t MaxPaths;
  size_t MaxLoopIterations;

  // 2. Add a simple constructor to initialize the members.
  // This allows the pass to be configured when instantiated.
  explicit PathEnumeratorPass(size_t maxPaths,
                              size_t maxLoopIterations)
      : MaxPaths(maxPaths), MaxLoopIterations(maxLoopIterations) {}
  PathEnumeratorPass() : MaxPaths(100), MaxLoopIterations(100) {}

  // The 'run' method for a Module Pass is correct as written.
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
};

} // namespace hepf

#endif // LLVM_CORE_PATHENUMERATORPASS_H
