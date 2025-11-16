#include "CriticalSectionTraversal.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

using namespace llvm;

// -----------------------------------------------------------
// Main Pass
// -----------------------------------------------------------

PreservedAnalyses CriticalSectionTraversalAlternativePass::run(Module &M, ModuleAnalysisManager &AM) {
    errs() << "=== Critical Section Alternative Analysis ===\n\n";

    size_t totalFunctions = 0;
    size_t totalBasicBlock = 0;
    size_t totalInstructions = 0;
    size_t totalcriticalSectionCount = 0;

    for (Function &F : M) {
        totalFunctions++;

        if (F.isDeclaration())
        continue;

        int criticalSectionCount = 0;
        int BasicBlockCount = 0;
        int InstructionCount = 0;
        for (BasicBlock &BB : F) {
        totalBasicBlock++;
        BasicBlockCount++;
        for (Instruction &I : BB) {
            totalInstructions++;
            InstructionCount++;
            if (auto *Call = dyn_cast<CallInst>(&I)) {
            if (Function *Callee = Call->getCalledFunction()) {
                std::string calleeName = Callee->getName().str();
                // Count critical section traversals -- just by name
                if (calleeName.find("mutex_lock") != std::string::npos || calleeName.find("spin_lock") != std::string::npos) {
                criticalSectionCount++;
                }
            }
            }
        }
        }

        // Per function output
        totalcriticalSectionCount += criticalSectionCount;
        errs() << "Function: " << F.getName() << ", Basic Blocks: " << BasicBlockCount << ", Instructions: " << InstructionCount << ", Critical Section Traversals: " << criticalSectionCount;
        if (InstructionCount > 0) {
            double percentage = (100.0 * criticalSectionCount) / InstructionCount;
            errs() << " (" << format("%.2f", percentage) << "%)";
        }
        errs() << "\n\n";
    }
    // Summary statistics
    errs() << "=== Summary ===\n";
    errs() << "Total Functions Analyzed: " << totalFunctions << "\n";
    errs() << "Functions with Critical Sections: " << totalcriticalSectionCount << "\n";
    errs() << "Total Instructions: " << totalInstructions << "\n";

    if (totalInstructions > 0) {
    double percentage = (100.0 * totalcriticalSectionCount) / totalInstructions;
    errs() << " (" << format("%.2f", percentage) << "%)";
    }
    errs() << "\n\n";

    errs().flush();
    return PreservedAnalyses::all();
}
