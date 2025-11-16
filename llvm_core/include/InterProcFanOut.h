#ifndef INTER_PROC_FAN_OUT_H
#define INTER_PROC_FAN_OUT_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class InterProcFanOutPass : public PassInfoMixin<InterProcFanOutPass> {
public:
  InterProcFanOutPass() = default;

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif // INTER_PROC_FAN_OUT_H
