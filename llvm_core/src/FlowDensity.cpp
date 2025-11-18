#include "FlowDensity.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/CallGraph.h"
#include <map>
#include <cmath> // Added for std::abs

using namespace llvm;

namespace hepf {

// -----------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------

// ASSUMPTION: The entropyMap in FlowDensity.h is now declared as:
// std::map<const Function*, float> entropyMap;

void FlowDensity::calculateEntropy(Module &M) {
    // Dummy entropy calculation
    for (auto &F : M) {
        if (!F.isDeclaration()) {
            // No const_cast needed if entropyMap uses const Function* as key
            entropyMap[&F] = F.getInstructionCount();
            errs() << "Function: " << F.getName();
        }
    }
}

// -----------------------------------------------------------
// Main Pass
// -----------------------------------------------------------

PreservedAnalyses FlowDensity::run(Module &M, ModuleAnalysisManager &AM) {
    errs() << "=== FlowDensity Analysis ===\n\n";

    calculateEntropy(M);
    // Retrieve CallGraph
    auto &CG = AM.getResult<CallGraphAnalysis>(M);

    float totalGradient = 0.0f;
    int edgeCount = 0;

    size_t node_num = 0;
    size_t edge_num = 0;

    // Iterate over all nodes in the CallGraph
    for (auto &node : CG) {
        const Function *F = node.first;

        node_num++;
        // Skip null functions and declarations
        if (F && !F->isDeclaration()) {

            // Check if F is in the map before using operator[]
            auto callerIt = entropyMap.find(F);
            if (callerIt == entropyMap.end()) {
                // Should not happen if calculateEntropy ran, but safer to check.
                continue;
            }
            float callerEntropy = callerIt->second;

            // Iterate over all edges (calls) from this function
            for (auto &edge : *node.second) {
                CallGraphNode *calleeNode = edge.second;
                const Function *callee = calleeNode->getFunction();

                edge_num++;
                // Skip null callees and declarations
                if (callee && !callee->isDeclaration()) {

                    auto calleeIt = entropyMap.find(callee);
                    if (calleeIt == entropyMap.end()) {
                        // Callee not in map (e.g., perhaps an indirect call to a function
                        // that calculateEntropy missed, although it shouldn't).
                        // For the optimization, we can assume that the callee has a
                        // default entropy value of 0.
                        continue;
                    }
                    float calleeEntropy = calleeIt->second;

                    // Calculate the absolute difference (gradient)
                    totalGradient += std::abs(callerEntropy - calleeEntropy);
                    edgeCount++;
                }
            }
        }
    }

    if (edgeCount > 0) {
        errs() << "Flow Density: " << totalGradient / edgeCount << "\n" << "Edge Count: " << edge_num << "\n" << "Node Count: " << node_num << "\n";
    } else {
        errs() << "Flow Density: 0\n" << "Edge Count: " << edge_num << "\n" << "Node Count: " << node_num << "\n";
    }
    errs().flush();

    return PreservedAnalyses::all();
}

} // namespace hepf
