#include "FeedbackResonance.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/IR/PassManager.h"
#include <map>

using namespace llvm;

namespace hepf {

// -----------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------

// The struct definition is in the header file.
// Only the implementation of the run method is here.

void FeedbackResonance::calculateEntropy(Module &M) {
    // Dummy entropy calculation
    for (auto &F : M) {
        if (!F.isDeclaration()) {
            entropyMap[&F] = F.getInstructionCount();
            errs() << "Function: " << F.getName();
        }
    }
}

bool FeedbackResonance::isHighEntropy(const Function *F) {
    if (entropyMap.count(F)) {
        return entropyMap[F] > entropyThreshold;
    }
    return false;
}

// -----------------------------------------------------------
// Main Pass
// -----------------------------------------------------------

PreservedAnalyses FeedbackResonance::run(Module &M, ModuleAnalysisManager &AM) {
    errs() << "=== FeedbackResonance Analysis ===\n\n";

    calculateEntropy(M);
    auto &CG = AM.getResult<CallGraphAnalysis>(M);

    size_t node_num = 0;

    for (scc_iterator<CallGraph *> I = scc_begin(&CG), E = scc_end(&CG); I != E; ++I) {
        const std::vector<CallGraphNode *> &SCC = *I;
        if (SCC.size() > 1) {
            bool hasHighEntropy = false;
            for (auto &node : SCC) {
                node_num++;
                Function *F = node->getFunction();
                if (F && isHighEntropy(F)) {
                    hasHighEntropy = true;
                    break;
                }
            }
            if (hasHighEntropy) {
                cyclicDependencies++;
            }
        }
    }
    errs() << "Feedback Resonance: " << cyclicDependencies << "\n" << "Nodes: " << node_num;
    errs().flush();
    return PreservedAnalyses::all();
}

} // namespace hepf
