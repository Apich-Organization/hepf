#include "PathEnumeratorPass.h"
#include "PathEnumerator.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace hepf {

PreservedAnalyses PathEnumeratorPass::run(Module &M, ModuleAnalysisManager &) {
  errs() << "=== Path Enumerator Pass ===\n\n";

  for (auto &F : M) {
    // Skip declarations and intrinsics
    if (F.isDeclaration()) {
      continue;
    }

    // Skip empty functions
    if (F.empty()) {
      errs() << "Skipping empty function: " << F.getName() << "\n";
      continue;
    }

    errs() << "Analyzing function: " << F.getName() << "\n";

    // Create path enumerator
    // Parameters: maxPaths = 10000, maxLoopIterations = 2
    // This will enumerate paths with loops traversed 0, 1, or 2 times
    PathEnumerator PE(F, MaxPaths, MaxLoopIterations);

    // Report results
    errs() << "  Paths found: " << PE.getPathCount();

    if (PE.hasReachedLimit()) {
      errs() << " (LIMIT REACHED - incomplete enumeration)";
    }

    errs() << "\n";

    // Optional: print paths for small path counts
    if (PE.getPathCount() > 0 && PE.getPathCount() <= 20) {
      const auto &paths = PE.getPaths();
      for (size_t i = 0; i < paths.size(); ++i) {
        errs() << "  Path " << (i + 1) << " (length " << paths[i].size()
               << "): ";
        for (size_t j = 0; j < paths[i].size(); ++j) {
          if (j > 0)
            errs() << " -> ";
          errs() << paths[i][j]->getName();
        }
        errs() << "\n";
      }
    } else if (PE.getPathCount() > 20) {
      errs() << "  (Too many paths to display individually)\n";
    }

    errs() << "\n";
  }

  errs().flush();
  return PreservedAnalyses::all();
}

} // namespace hepf
