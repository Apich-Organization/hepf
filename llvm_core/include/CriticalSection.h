#ifndef LLVM_CRITICALSECTIONTRAVERSAL_H
#define LLVM_CRITICALSECTIONTRAVERSAL_H

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include <cstdint>
#include <map>
#include <set>

namespace llvm {

// The domain is the set of lock names (llvm::Value) currently held.
using LockSet = std::set<const llvm::Value *>;

class CriticalSectionTraversalPass
    : public PassInfoMixin<CriticalSectionTraversalPass> {
public:
  CriticalSectionTraversalPass() = default;

  explicit CriticalSectionTraversalPass(std::int64_t Offset)
      : OffsetValue(Offset) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);

private:
  // Helper function to calculate the lock state after an instruction
  LockSet runOnInstruction(const Instruction &I, const LockSet &In,
                           std::int64_t &Offset);

  // Core Data-Flow Analysis implementation
  void computeHeldLocks(Function &F,
                        std::map<const BasicBlock *, LockSet> &InStates,
                        std::int64_t &Offset);

  // Offset value
  std::int64_t OffsetValue;
};

} // end namespace llvm

#endif // LLVM_CRITICALSECTIONTRAVERSAL_H
