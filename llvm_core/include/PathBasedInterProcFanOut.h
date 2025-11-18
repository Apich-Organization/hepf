#ifndef LLVM_CORE_PATHBASEDINTERPROCFANOUT_H
#define LLVM_CORE_PATHBASEDINTERPROCFANOUT_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace hepf {

struct PathBasedInterProcFanOutPass
    : public llvm::PassInfoMixin<PathBasedInterProcFanOutPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
};

} // namespace hepf

#endif // LLVM_CORE_PATHBASEDINTERPROCFANOUT_H
