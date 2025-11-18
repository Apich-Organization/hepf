#ifndef LLVM_CORE_PATHBASEDMAXPATH_H
#define LLVM_CORE_PATHBASEDMAXPATH_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace hepf {

struct PathBasedMaxPathPass : public llvm::PassInfoMixin<PathBasedMaxPathPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
};

} // namespace hepf

#endif // LLVM_CORE_PATHBASEDMAXPATH_H
