#include "InterProcFanOut.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <set>
#include <map>

using namespace llvm;

PreservedAnalyses InterProcFanOutPass::run(Module &M, ModuleAnalysisManager &AM) {
    errs() << "=== Inter-Process FanOut Analysis ===\n\n";

    // 1. Get the CallGraph
    CallGraph &CG = AM.getResult<CallGraphAnalysis>(M);

    // Get the special external node for calls to unknown/unresolved functions
    const CallGraphNode *ExternalNode = CG.getExternalCallingNode();

    // Store the results to ensure we process all functions once
    std::map<const Function *, int> fanOutResults;

    // --- Phase 1: Calculate Fan-Out for Defined Functions ---
    for (auto &Node : CG) {
        // Node.first is the calling function (Function*)
        if (Function *F = const_cast<Function*>(Node.first)) {

            // Only process functions defined within this module
            if (F->isDeclaration() || F->isIntrinsic()) {
                continue;
            }

            // Set to track unique *resolved* function targets
            std::set<const Function *> resolvedTargets;
            bool callsUnresolvedExternal = false;

            // Get the CallGraphNode corresponding to the current function F
            const CallGraphNode *CGN = Node.second.get();

            // Iterate over all outgoing edges from this node (all calls made by F)
            for (const auto &Edge : *CGN) {
                const CallGraphNode *CalleeNode = Edge.second;

                if (Function *Callee = CalleeNode->getFunction()) {
                    // This is a resolved call target (either defined or declared).
                    // We only count it if it's not the function calling itself (self-recursion)
                    if (Callee != F) {
                        resolvedTargets.insert(Callee);
                    }
                } else if (CalleeNode == ExternalNode) {
                    // This is a call whose target could not be resolved by the CallGraph,
                    // typically an indirect call or a call to a function not in the module.
                    // Counting this uniquely improves robustness.
                    callsUnresolvedExternal = true;
                }
            }

            int fan_out = resolvedTargets.size();

            // Add 1 if the function makes any calls to unresolved external targets.
            // This treats the entire set of unresolved calls as a single unique coupling.
            if (callsUnresolvedExternal) {
                fan_out += 1;
            }

            fanOutResults[F] = fan_out;
        }
    }

    // --- Phase 2: Output Results ---
    for (const auto& pair : fanOutResults) {
        errs() << "Function: " << pair.first->getName() << ", Fan-out: " << pair.second << "\n";
    }

    errs().flush();

    // The pass only performs analysis.
    return PreservedAnalyses::all();
}
