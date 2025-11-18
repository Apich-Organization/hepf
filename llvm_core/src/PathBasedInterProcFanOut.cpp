#include "PathBasedInterProcFanOut.h"
#include "PathEnumerator.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <unordered_set>

using namespace llvm;
using namespace hepf;

namespace {

// Proper taint analysis within a path
class PathTaintAnalysis {
public:
  PathTaintAnalysis(const Path &path) {
    // Initialize: function arguments are tainted (user-controlled)
    if (!path.empty() && path[0]->getParent()) {
      Function *F = path[0]->getParent();
      for (Argument &Arg : F->args()) {
        tainted.insert(&Arg);
      }
    }

    // Propagate taint through the path
    for (BasicBlock *BB : path) {
      for (Instruction &I : *BB) {
        propagateTaint(&I);
      }
    }
  }

  bool isTainted(Value *V) const { return tainted.count(V) > 0; }

private:
  // -----------------------------------------------------------
  // Helper Functions
  // -----------------------------------------------------------
  void propagateTaint(Instruction *I) {
    bool anyOperandTainted = false;

    // Check if any operand is tainted
    for (Use &U : I->operands()) {
      if (tainted.count(U.get())) {
        anyOperandTainted = true;
        break;
      }
    }

    // Propagate taint based on instruction type
    if (auto *Load = dyn_cast<LoadInst>(I)) {
      // If loading from a tainted address, result is tainted
      if (tainted.count(Load->getPointerOperand())) {
        tainted.insert(Load);
      }
    } else if (auto *Store = dyn_cast<StoreInst>(I)) {
      // Store doesn't produce a value, but taints memory location
      // This is tracked separately
    } else if (auto *Call = dyn_cast<CallInst>(I)) {
      Function *Callee = Call->getCalledFunction();
      if (Callee) {
        StringRef Name = Callee->getName();
        // Input functions produce tainted data
        if (Name.contains("read") || Name.contains("recv") ||
            Name.contains("get") || Name.contains("scan") ||
            Name.contains("input") || Name.contains("fread") ||
            Name.contains("getchar") || Name.contains("fgets")) {
          tainted.insert(Call);
        } else if (anyOperandTainted) {
          // If any argument is tainted, result might be tainted
          // (conservative assumption)
          tainted.insert(Call);
        }
      } else if (anyOperandTainted) {
        // Indirect call with tainted operands
        tainted.insert(Call);
      }
    } else if (auto *Phi = dyn_cast<PHINode>(I)) {
      // PHI node is tainted if any incoming value is tainted
      for (unsigned i = 0; i < Phi->getNumIncomingValues(); ++i) {
        if (tainted.count(Phi->getIncomingValue(i))) {
          tainted.insert(Phi);
          break;
        }
      }
    } else if (anyOperandTainted) {
      // For most instructions, if any operand is tainted, result is tainted
      if (!I->getType()->isVoidTy()) {
        tainted.insert(I);
      }
    }
  }

  std::unordered_set<Value *> tainted;
};

// Calculate fan-out for a single path
unsigned calculatePathFanOut(const Path &path) {
  if (path.empty())
    return 0;

  // First, perform taint analysis on this path
  PathTaintAnalysis TaintAnalysis(path);

  unsigned fanOut = 0;
  std::unordered_set<Function *> calledFunctions;

  for (BasicBlock *BB : path) {
    for (Instruction &I : *BB) {
      // Check for call instructions (both direct and indirect)
      CallInst *Call = dyn_cast<CallInst>(&I);
      if (!Call)
        continue;

      Function *Callee = Call->getCalledFunction();

      // Skip intrinsics but count other calls
      if (Callee && Callee->isIntrinsic()) {
        continue;
      }

      // For indirect calls, we count them if they have tainted operands
      bool isIndirect = (Callee == nullptr);

      // Check if this call has tainted arguments
      bool hasTaintedArg = false;
      for (unsigned i = 0; i < Call->arg_size(); ++i) {
        Value *Arg = Call->getArgOperand(i);
        if (TaintAnalysis.isTainted(Arg)) {
          hasTaintedArg = true;
          break;
        }
      }

      // Count this call if:
      // 1. It has tainted arguments, AND
      // 2. We haven't counted this function yet (for direct calls), OR
      //    it's an indirect call (count each occurrence)
      if (hasTaintedArg) {
        if (isIndirect) {
          // For indirect calls, count each occurrence
          fanOut++;
        } else {
          // For direct calls, deduplicate by function
          if (calledFunctions.insert(Callee).second) {
            fanOut++;
          }
        }
      }
    }
  }

  return fanOut;
}

} // anonymous namespace

// -----------------------------------------------------------
// Main Pass
// -----------------------------------------------------------
PreservedAnalyses PathBasedInterProcFanOutPass::run(Module &M,
                                                    ModuleAnalysisManager &AM) {
  size_t function_num = 0;

  for (Function &F : M) {
    function_num++;
    // Skip declarations, empty functions, and intrinsics
    if (F.isDeclaration() || F.empty() || F.isIntrinsic()) {
      continue;
    }

    errs() << "Analyzing function: " << F.getName() << "\n";

    // Create path enumerator with reasonable limits
    // Use smaller maxLoopIterations (1) for fan-out analysis to avoid explosion
    PathEnumerator PE(F, 5000, 1);

    if (PE.getPathCount() == 0) {
      errs() << "  No paths found (function may have no exits)\n\n";
      continue;
    }

    const auto &paths = PE.getPaths();

    // Track statistics
    unsigned maxFanOut = 0;
    unsigned totalFanOut = 0;
    size_t pathsAnalyzed = 0;

    // Limit detailed output for functions with many paths
    bool printDetails = paths.size() <= 50;

    for (size_t i = 0; i < paths.size(); ++i) {
      unsigned fanOut = calculatePathFanOut(paths[i]);

      maxFanOut = std::max(maxFanOut, fanOut);
      totalFanOut += fanOut;
      pathsAnalyzed++;

      if (printDetails) {
        errs() << "  Path " << (i + 1) << " (length: " << paths[i].size()
               << " blocks): FanOut = " << fanOut << "\n";
      }
    }

    // Print summary
    errs() << "  Summary for " << F.getName() << ":\n";
    errs() << "    Total functions analyzed: " << pathsAnalyzed << "\n";
    errs() << "    Total paths analyzed: " << pathsAnalyzed << "\n";
    errs() << "    Maximum fan-out: " << maxFanOut << "\n";

    if (pathsAnalyzed > 0) {
      errs() << "    Average fan-out: "
             << (static_cast<double>(totalFanOut) / pathsAnalyzed) << "\n";
    }

    if (PE.hasReachedLimit()) {
      errs() << "    Warning: Path limit reached, analysis incomplete\n";
    }

    errs() << "\n";
  }

  errs().flush();
  return PreservedAnalyses::all();
}
