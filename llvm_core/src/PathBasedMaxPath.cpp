#include "PathBasedMaxPath.h"
#include "PathEnumerator.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace llvm;
using namespace hepf;

namespace {

// Build a path-aware dependence graph
class PathDependenceGraph {
public:
  PathDependenceGraph(const Path &path, DependenceInfo *DI) : DI(DI) {
    collectInstructions(path);
    buildDependencies();
  }

  // Calculate the longest dependency chain (critical path length)
  unsigned getLongestPath() {
    if (instructions.empty())
      return 0;

    std::unordered_map<Instruction *, unsigned> memo;
    unsigned maxPath = 0;

    for (Instruction *I : instructions) {
      maxPath = std::max(maxPath, getLongestPathFrom(I, memo));
    }

    return maxPath;
  }

private:
  // -----------------------------------------------------------
  // Helper Functions
  // -----------------------------------------------------------
  void collectInstructions(const Path &path) {
    for (BasicBlock *BB : path) {
      for (Instruction &I : *BB) {
        instructions.push_back(&I);
        instToIndex[&I] = instructions.size() - 1;
      }
    }
  }

  void buildDependencies() {
    adjacencyList.resize(instructions.size());

    // Build a set of instructions in this path for quick lookup
    std::unordered_set<Instruction *> pathInsts(instructions.begin(),
                                                instructions.end());

    for (size_t i = 0; i < instructions.size(); ++i) {
      Instruction *I = instructions[i];

      // 1. Data dependencies through operands (including PHI nodes)
      for (Use &U : I->operands()) {
        if (auto *OpInst = dyn_cast<Instruction>(U.get())) {
          // Only add edge if the operand is in this path
          if (pathInsts.count(OpInst)) {
            size_t opIdx = instToIndex[OpInst];
            adjacencyList[opIdx].push_back(i);
          }
        }
      }

      // 2. Memory dependencies - must be path-aware
      if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
        // Check all earlier memory operations in the path
        for (size_t j = 0; j < i; ++j) {
          Instruction *Earlier = instructions[j];

          if (!isa<LoadInst>(Earlier) && !isa<StoreInst>(Earlier)) {
            continue;
          }

          bool hasMemDep = false;

          // Use DependenceInfo if available, but only for same BB or provable
          // deps
          if (DI && Earlier->getParent() == I->getParent()) {
            if (auto Dep = DI->depends(Earlier, I, true)) {
              hasMemDep = true;
            }
          } else {
            // Conservative: assume memory dependency for cross-BB operations
            // This is the safest approach for path-based analysis
            // We could refine this with more sophisticated alias analysis

            // Load-Load: no dependency
            if (isa<LoadInst>(Earlier) && isa<LoadInst>(I)) {
              hasMemDep = false;
            }
            // Store-Load, Store-Store, Load-Store: potential dependency
            else {
              hasMemDep = true;
            }
          }

          if (hasMemDep) {
            adjacencyList[j].push_back(i);
          }
        }
      }

      // 3. Control dependencies (implicit in path structure)
      // Instructions in later blocks depend on terminators of earlier blocks
      if (i > 0 && I->getParent() != instructions[i - 1]->getParent()) {
        // We crossed a basic block boundary
        // Find the terminator of the previous block
        BasicBlock *PrevBB = instructions[i - 1]->getParent();
        Instruction *Term = PrevBB->getTerminator();

        if (Term && pathInsts.count(Term)) {
          size_t termIdx = instToIndex[Term];
          adjacencyList[termIdx].push_back(i);
        }
      }

      // 4. Special handling for alloca and other memory-related calls
      if (auto *Call = dyn_cast<CallInst>(I)) {
        Function *Callee = Call->getCalledFunction();

        // Memory allocation/deallocation creates dependencies
        if (Callee) {
          StringRef Name = Callee->getName();
          if (Name.contains("alloc") || Name.contains("free") ||
              Name.contains("realloc") || Name.contains("malloc")) {
            // All subsequent memory operations depend on this
            for (size_t j = i + 1; j < instructions.size(); ++j) {
              if (isa<LoadInst>(instructions[j]) ||
                  isa<StoreInst>(instructions[j])) {
                adjacencyList[i].push_back(j);
              }
            }
          }
        }
      }
    }
  }

  unsigned
  getLongestPathFrom(Instruction *I,
                     std::unordered_map<Instruction *, unsigned> &memo) {
    // Check if already computed
    auto it = memo.find(I);
    if (it != memo.end()) {
      return it->second;
    }

    unsigned maxLength = 1; // At least the instruction itself

    auto idxIt = instToIndex.find(I);
    if (idxIt != instToIndex.end()) {
      size_t idx = idxIt->second;

      // Check all dependent instructions
      for (size_t depIdx : adjacencyList[idx]) {
        if (depIdx < instructions.size()) { // Safety check
          Instruction *DepInst = instructions[depIdx];
          unsigned pathLength = 1 + getLongestPathFrom(DepInst, memo);
          maxLength = std::max(maxLength, pathLength);
        }
      }
    }

    memo[I] = maxLength;
    return maxLength;
  }

  DependenceInfo *DI;
  std::vector<Instruction *> instructions;
  std::unordered_map<Instruction *, size_t> instToIndex;
  std::vector<std::vector<size_t>> adjacencyList;
};

} // anonymous namespace

// -----------------------------------------------------------
// Main Pass
// -----------------------------------------------------------
PreservedAnalyses PathBasedMaxPathPass::run(Module &M,
                                            ModuleAnalysisManager &AM) {
  errs() << "=== Path Based Max Path Pass ===\n\n";

  size_t function_num = 0;

  for (Function &F : M) {
    function_num++;
    // Skip declarations, empty functions, and intrinsics
    if (F.isDeclaration() || F.empty() || F.isIntrinsic()) {
      continue;
    }

    errs() << "Analyzing function: " << F.getName() << "\n";

    // Create path enumerator with reasonable limits
    PathEnumerator PE(F, 5000, 1);

    if (PE.getPathCount() == 0) {
      errs() << "  No paths found (function may have no exits)\n\n";
      continue;
    }

    // Try to get dependence analysis results
    DependenceInfo *DI = nullptr;
    auto &FAM =
        AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    try {
      DI = &FAM.getResult<DependenceAnalysis>(F);
    } catch (...) {
      errs() << "  Info: Using conservative memory dependencies "
             << "(DependenceAnalysis not available)\n";
    }

    const auto &paths = PE.getPaths();

    // Track statistics
    unsigned maxPathLength = 0;
    unsigned totalPathLength = 0;
    size_t pathsAnalyzed = 0;

    // Limit detailed output for functions with many paths
    bool printDetails = paths.size() <= 50;

    for (size_t i = 0; i < paths.size(); ++i) {
      // Build dependence graph for this path
      PathDependenceGraph PDG(paths[i], DI);
      unsigned pathLength = PDG.getLongestPath();

      maxPathLength = std::max(maxPathLength, pathLength);
      totalPathLength += pathLength;
      pathsAnalyzed++;

      if (printDetails) {
        errs() << "  Path " << (i + 1) << " (BB count: " << paths[i].size()
               << ", critical path: " << pathLength << " instructions)\n";
      }
    }

    // Print summary
    errs() << "  Summary for " << F.getName() << ":\n";
    errs() << "    Total functions analyzed: " << function_num << "\n";
    errs() << "    Total paths analyzed: " << pathsAnalyzed << "\n";
    errs() << "    Maximum critical path length: " << maxPathLength
           << " instructions\n";

    if (pathsAnalyzed > 0) {
      errs() << "    Average critical path length: "
             << (static_cast<double>(totalPathLength) / pathsAnalyzed)
             << " instructions\n";
    }

    if (PE.hasReachedLimit()) {
      errs() << "    Warning: Path limit reached, analysis incomplete\n";
    }

    errs() << "\n";
  }

  errs().flush();
  return PreservedAnalyses::all();
}
