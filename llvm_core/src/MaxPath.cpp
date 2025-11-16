#include "MaxPath.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/STLExtras.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <iterator>

using namespace llvm;

namespace {

// -----------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------

// Helper function to recursively compute the longest path from a given instruction
int computeLongestPath(Instruction *I,
                       std::unordered_map<Instruction *, int> &memo,
                       std::unordered_set<Instruction *> &visiting,
                       DependenceInfo &DI) {

    // 1. Cycle Detection (Crucial for handling loops and recurrences)
    if (visiting.count(I)) {
        return 0; // Cut the back-edge.
    }

    // 2. Memoization
    if (memo.count(I)) {
        return memo[I];
    }

    visiting.insert(I);
    int maxLength = 0;

    // A. Iterate over uses (Data Dependence / Use-Def Chain)
    // This handles all value flow dependencies (X = Y + Z; User = X).
    for (auto &use : I->uses()) {
        if (Instruction *user = dyn_cast<Instruction>(use.getUser())) {
            // Data dependence always constitutes a valid path edge.
            maxLength = std::max(maxLength,
                                 computeLongestPath(user, memo, visiting, DI));
        }
    }

    // B. Check for Memory Dependencies
        if (I->mayReadOrWriteMemory()) {

            // --- 1. Iterate through the rest of the current BasicBlock ---
            // Get the iterator pointing to the instruction immediately following I
            BasicBlock::iterator nextInst = I->getIterator();
            ++nextInst;

            for (; nextInst != I->getParent()->end(); ++nextInst) {
                Instruction *user = &*nextInst;

                // Check if the potential dependent accesses memory and is not a simple data-flow use
                if (user->mayReadOrWriteMemory() && !llvm::is_contained(user->operands(), I)) {

                    std::unique_ptr<llvm::Dependence> Dep = DI.depends(I, user, true);
                    if (Dep) {
                        // Update maxLength with the longest path found through this memory dependence edge
                        maxLength = std::max(maxLength,
                                             1 + computeLongestPath(user, memo, visiting, DI));
                    }
                }
            }

            // --- 2. Iterate through all subsequent BasicBlocks in the Function ---
            Function *F = I->getFunction();
            // Start from the basic block immediately following I's parent block
            auto BB_start = std::next(I->getParent()->getIterator());

            for (auto BBI = BB_start, E = F->end(); BBI != E; ++BBI) {
                for (Instruction &user : *BBI) {

                    // Check if the potential dependent accesses memory and is not a simple data-flow use
                    if (user.mayReadOrWriteMemory() && !llvm::is_contained(user.operands(), I)) {

                        std::unique_ptr<llvm::Dependence> Dep = DI.depends(I, &user, true);
                        if (Dep) {
                            maxLength = std::max(maxLength,
                                                 1 + computeLongestPath(&user, memo, visiting, DI));
                        }
                    }
                }
            }
        }

    // --- Backtrack and Store ---
    visiting.erase(I);

    // The path length is 1 (for I itself) + the longest path from its successors
    int pathLength = 1 + maxLength;
    memo[I] = pathLength;
    return pathLength;
}

} // namespace

// -----------------------------------------------------------
// Main Pass
// -----------------------------------------------------------

// --- MaxPathPass Implementation ---

PreservedAnalyses MaxPathPass::run(Module &M, ModuleAnalysisManager &AM) {
    errs() << "=== MaxPath Analysis ===\n\n";

    size_t totalFunctions = 0;
    size_t totalBasicBlock = 0;
    size_t totalInstructions = 0;

    FunctionAnalysisManager &FAM =
        AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    for (Function &F : M) {
        totalFunctions++;

        if (F.isDeclaration())
            continue;

        // Get the DependenceInfo for the function
        DependenceInfo &DI = FAM.getResult<DependenceAnalysis>(F);

        std::unordered_map<Instruction *, int> memo;
        int maxPath = 0;
        int BasicBlockCount = 0;
        int InstructionCount = 0;

        for (BasicBlock &BB : F) {
            totalBasicBlock++;
            BasicBlockCount++;

            for (Instruction &I : BB) {
                totalInstructions++;
                InstructionCount++;

                // The visiting set is local to the current DFS call stack
                std::unordered_set<Instruction *> visiting;

                maxPath = std::max(maxPath, computeLongestPath(&I, memo, visiting, DI));
            }
        }

        // Print the result expected by the test harness
        errs() << "Function: " << F.getName() << ", Basic Blocks: " << BasicBlockCount << ", Instructions: " << InstructionCount << ", MaxPath: " << maxPath << "\n";
        errs().flush();

        // Add a metadata node to the function to mark it as modified
        LLVMContext &C = F.getContext();
        MDNode *N = MDNode::get(C, MDString::get(C, "hepf.maxpath"));
        F.setMetadata("hepf", N);
    }

    // The analysis modified the function/added metadata, so no analyses are preserved.
    return PreservedAnalyses::none();
}
