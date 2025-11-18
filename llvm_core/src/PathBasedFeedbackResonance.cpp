#include "PathBasedFeedbackResonance.h"
#include "PathEnumerator.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/BasicBlock.h" // Needed for BasicBlock::size()
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm> // For std::max
#include <map>

using namespace llvm;
using namespace hepf;

// -----------------------------------------------------------
// Main Pass
// -----------------------------------------------------------
PreservedAnalyses
PathBasedFeedbackResonancePass::run(Module &M, ModuleAnalysisManager &AM) {

  // 1. Calculate Function "Entropy" (Instruction Count)
  std::map<Function *, float> functionEntropyMap;

  for (auto &Func : M) {
    if (!Func.isDeclaration()) {
      size_t instructionCount = 0;
      for (const BasicBlock &BB : Func) {
        instructionCount += BB.size();
      }
      functionEntropyMap[&Func] = static_cast<float>(instructionCount);
    }
  }

  // 2. Analyze the Call Graph
  auto &CG = AM.getResult<CallGraphAnalysis>(M);

  int highEntropyCyclicDependencies = 0;

  // Iterate over all Strongly Connected Components (SCCs)
  for (scc_iterator<CallGraph *> I = scc_begin(&CG), E = scc_end(&CG); I != E;
       ++I) {
    const std::vector<CallGraphNode *> &SCC = *I;
    errs() << "SCC Size: " << SCC.size() << "\n";

    // An SCC with size > 1 represents mutual recursion.
    // An SCC with size 1 represents either a non-recursive function or direct
    // recursion (self-loop).

    // Find the maximum entropy within this SCC
    float maxEntropy = 0.0f;
    bool hasFunction = false;

    for (auto &node : SCC) {
      Function *cycleF = node->getFunction();
      if (cycleF) {
        hasFunction = true;

        // Lookup and print entropy
        float entropy = 0.0f;
        if (functionEntropyMap.count(cycleF)) {
          entropy = functionEntropyMap[cycleF];
        }
        errs() << "  Function in SCC: " << cycleF->getName()
               << ", Entropy: " << entropy << "\n";

        // Update max entropy for the SCC
        maxEntropy = std::max(maxEntropy, entropy);
      }
    }

    if (!hasFunction)
      continue; // Skip if the SCC consists only of external nodes/libraries

    // 3. Check for High-Entropy Cycles

    if (SCC.size() > 1) { // Mutual Recursion
      if (maxEntropy > Threshold) {
        highEntropyCyclicDependencies++;
      }
    } else if (SCC.size() ==
               1) { // Single-Node SCC (Check for self-loop/direct recursion)
      CallGraphNode *node = SCC[0];
      Function *cycleF = node->getFunction();

      // Check for a self-loop (direct recursion)
      bool hasSelfLoop = false;
      if (cycleF) {

        for (auto &edge : *node) {
          if (edge.second->getFunction() == cycleF) {
            hasSelfLoop = true;
            break;
          }
        }
      }

      if (hasSelfLoop && maxEntropy > Threshold) {
        highEntropyCyclicDependencies++;
      }
    }
  }

  // 4. Report Results
  errs() << "High Entropy Cyclic Dependencies: "
         << highEntropyCyclicDependencies << "\n";
  errs().flush();

  // No transformation is done, so we preserve all analyses.
  return PreservedAnalyses::all();
}
