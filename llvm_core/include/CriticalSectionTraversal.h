#ifndef CRITICAL_SECTION_TRAVERSAL_H
#define CRITICAL_SECTION_TRAVERSAL_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class CriticalSectionTraversalAlternativePass : public PassInfoMixin<CriticalSectionTraversalAlternativePass> {
public:
  CriticalSectionTraversalAlternativePass() = default;

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif // CRITICAL_SECTION_TRAVERSAL_H
