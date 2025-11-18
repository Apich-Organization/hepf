#include "PathBasedCriticalSectionTraversal.h"
#include "PathEnumerator.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <set>

using namespace llvm;
using namespace hepf;

// -----------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------
// Define known lock and unlock function names for robustness.
static const std::set<StringRef> LockFunctions = {
    "mutex_lock", "spin_lock", "pthread_mutex_lock", "acquire_lock"};

static const std::set<StringRef> UnlockFunctions = {
    "mutex_unlock", "spin_unlock", "pthread_mutex_unlock", "release_lock"};

// ---

// -----------------------------------------------------------
// Main Pass
// -----------------------------------------------------------
PathBasedCriticalSectionTraversalPass::PathBasedCriticalSectionTraversalPass(
    size_t maxPaths,
    size_t maxLoopIterations)
    : MaxPaths(maxPaths),
      MaxLoopIterations(maxLoopIterations)
{
    // Difinition of the constructor.
}

PreservedAnalyses
PathBasedCriticalSectionTraversalPass::run(Module &M, ModuleAnalysisManager &) {

  size_t auto_count = 0;
  for (auto &F : M) {
      auto_count++;
    // Only analyze function definitions
    if (F.isDeclaration() || F.empty()) {
      continue;
    }

    // 1. Initialize the PathEnumerator with the function and the path limit.
    PathEnumerator PE(F, MaxPaths, MaxLoopIterations);

    unsigned path_count = 0;

    // Use the iterator returned by getPaths().
    for (const auto &path : PE.getPaths()) {

      // Critical Section Depth: 0 = outside, 1+ = inside.
      int criticalSectionDepth = 0;

      // Core analysis: Iterate through instructions in each basic block of the
      // path
      for (auto *bb : path) {
        for (auto &I : *bb) {
          if (auto *call = dyn_cast<CallInst>(&I)) {

            // Check for direct calls only (Still ignores indirect calls, a
            // known limitation)
            if (Function *calledFunc = call->getCalledFunction()) {
              StringRef name = calledFunc->getName();

              // Check if the call is a LOCK operation
              if (LockFunctions.count(name)) {
                criticalSectionDepth++;
              }
              // Check if the call is an UNLOCK operation
              else if (UnlockFunctions.count(name)) {
                criticalSectionDepth--;
              }
            }
          }
        }
      }

      // 2. Report Analysis Results
      errs() << "Function: " << F.getName() << ", Path (BBs): [";

      bool first = true;
      for (auto *bb : path) {
        if (!first) {
          errs() << " -> ";
        }
        // Use printAsOperand to get the unique ID for the BasicBlock
        bb->printAsOperand(errs(), false);
        first = false;
      }
      errs() << "]";

      errs() << ", Final Lock Depth: " << criticalSectionDepth;

      // 3. Highlight potential critical problems
      if (criticalSectionDepth != 0) {
        errs() << " **(WARNING: Unbalanced Lock/Unlock Pair)**";
      }
      errs() << "\n";

      path_count++;
    }

    // Output a warning if the path limit was reached.
    if (path_count == MaxPaths) {
      errs() << "WARNING: Path limit of " << MaxPaths
             << " reached for function " << F.getName()
             << ". Analysis may be incomplete.\n";
    }
  }

  errs().flush();
  return PreservedAnalyses::all();
}
