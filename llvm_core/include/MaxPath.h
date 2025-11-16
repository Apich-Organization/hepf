#ifndef MAX_PATH_PASS_H
#define MAX_PATH_PASS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class MaxPathPass : public PassInfoMixin<MaxPathPass> {
public:
  MaxPathPass() = default;

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif // MAX_PATH_PASS_H
