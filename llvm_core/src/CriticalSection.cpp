#include "CriticalSection.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <deque>
#include <set>
#include <map>

using namespace llvm;

// --- Target Lock/Unlock Function Definitions ---
static const std::set<std::string> TargetLocks = {
    "mutex_lock", "spin_lock", "pthread_mutex_lock",
    "mtx_lock", "_lock_acquire", "acquire_lock"
};

static const std::set<std::string> TargetUnlocks = {
    "mutex_unlock", "spin_unlock", "pthread_mutex_unlock",
    "mtx_unlock", "_lock_release", "release_lock"
};

// Try-lock functions that may or may not acquire a lock
static const std::set<std::string> TryLocks = {
    "pthread_mutex_trylock", "mutex_trylock", "spin_trylock"
};

// -----------------------------------------------------------
// Helper: Check if a function name matches lock/unlock patterns
// -----------------------------------------------------------
static bool isLockFunction(const std::string& name) {
    if (TargetLocks.count(name)) return true;

    // Pattern matching for common lock function names
    return name.find("lock") != std::string::npos &&
           name.find("unlock") == std::string::npos &&
           name.find("trylock") == std::string::npos;
}

static bool isUnlockFunction(const std::string& name) {
    if (TargetUnlocks.count(name)) return true;

    // Pattern matching for common unlock function names
    return name.find("unlock") != std::string::npos ||
           name.find("release") != std::string::npos;
}

static bool isTryLockFunction(const std::string& name) {
    if (TryLocks.count(name)) return true;

    return name.find("trylock") != std::string::npos ||
           name.find("try_lock") != std::string::npos;
}

// -----------------------------------------------------------
// Helper: Get a stable identifier for a lock value
// -----------------------------------------------------------
static const Value* getCanonicalLockIdentifier(const Value* V) {
    // Strip casts and simple operations to get to the base lock object
    while (V) {
        if (auto *Cast = dyn_cast<CastInst>(V)) {
            V = Cast->getOperand(0);
        } else if (auto *GEP = dyn_cast<GetElementPtrInst>(V)) {
            // For simple GEPs, use the base pointer
            V = GEP->getPointerOperand();
        } else {
            break;
        }
    }
    return V;
}

// -----------------------------------------------------------
// 1. Transfer Function
// -----------------------------------------------------------

LockSet CriticalSectionTraversalPass::runOnInstruction(
    const Instruction& I,
    const LockSet& In,
    std::int64_t& Offset)
{
    LockSet Out = In;

    if (auto *Call = dyn_cast<CallInst>(&I)) {
        Function *Callee = Call->getCalledFunction();

        // Handle direct calls
        if (Callee && !Callee->isIntrinsic()) {
            std::string calleeName = Callee->getName().str();

            // Skip if no arguments (can't be a lock operation)
            if (Call->arg_size() < 1) {
                return Out;
            }

            const Value* LockIdentifier = Call->getArgOperand(0);

            // Skip null or undef lock identifiers
            if (isa<UndefValue>(LockIdentifier) || isa<ConstantPointerNull>(LockIdentifier)) {
                return Out;
            }

            // Get canonical identifier to handle casts/conversions
            LockIdentifier = getCanonicalLockIdentifier(LockIdentifier);

            if (isLockFunction(calleeName)) {
                // LOCK ACQUISITION
                Out.insert(LockIdentifier);

            } else if (isUnlockFunction(calleeName)) {
                // LOCK RELEASE
                // Remove this specific lock
                Out.erase(LockIdentifier);

            } else if (isTryLockFunction(calleeName)) {
                if (Offset == 0) {
                    Out.insert(LockIdentifier);
                } else if (Offset == 1){
                    Out.insert(LockIdentifier);
                } else if (Offset == 2){
                } else {
                }
                // TRY-LOCK: Conservative approach
                // Don't add the lock since acquisition is conditional
                // A more sophisticated analysis would track the return value
            }

        } else if (!Callee) {
            if (Offset == 0) {
                Out.clear();
            } else if (Offset == 1) {
            } else if (Offset == 2) {
                Out.clear();
            } else {
            }
            // INDIRECT CALL HANDLING
            // Conservative approach for soundness
            // Option 1: Clear all locks (most conservative - assumes any lock could be released)
            // Option 2: Leave unchanged (optimistic - assumes no lock operations)
            // Option 3: Use heuristics based on function pointer types
        }
    }

    return Out;
}

// -----------------------------------------------------------
// 2. Data Flow Analysis
// -----------------------------------------------------------

void CriticalSectionTraversalPass::computeHeldLocks(
    Function &F,
    std::map<const BasicBlock*, LockSet>& InStates,
    std::int64_t& Offset)
{
    std::deque<BasicBlock*> Worklist;
    std::set<BasicBlock*> InWorklist;
    std::map<const BasicBlock*, LockSet> OutStates;

    BasicBlock *EntryBB = &(F.getEntryBlock());

    // 1. Initialize States
    for (BasicBlock &BB : F) {
        InStates[&BB] = LockSet();
        OutStates[&BB] = LockSet();
    }

    // Entry block starts with empty lock set (no locks held initially)
    InStates[EntryBB] = LockSet();

    // Initialize worklist with all blocks
    for (BasicBlock &BB : F) {
        Worklist.push_back(&BB);
        InWorklist.insert(&BB);
    }

    auto AddToWorklist = [&](BasicBlock *BB) {
        if (InWorklist.insert(BB).second) {
            Worklist.push_back(BB);
        }
    };

    // 2. Fixed-Point Iteration
    size_t iterations = 0;
    const size_t MAX_ITERATIONS = 10000; // Prevent infinite loops

    while (!Worklist.empty() && iterations < MAX_ITERATIONS) {
        iterations++;

        BasicBlock *BB = Worklist.front();
        Worklist.pop_front();
        InWorklist.erase(BB);

        // 2a. Compute New InState (Meet = Union of predecessor OutStates)
        LockSet NewInState;

        if (BB != EntryBB) {
            bool firstPred = true;

            for (BasicBlock *Pred : predecessors(BB)) {
                const LockSet& PredOutState = OutStates[Pred];

                if (firstPred) {
                    NewInState = PredOutState;
                    firstPred = false;
                } else {
                    // Union: A lock is held if it's held on ANY path
                    // This is a may-analysis (over-approximation)
                    NewInState.insert(PredOutState.begin(), PredOutState.end());
                }
            }

            // Handle unreachable blocks
            if (firstPred) {
                // No predecessors, block is unreachable
                NewInState.clear();
            }
        }
        // Entry block stays empty

        // Check for convergence
        if (NewInState == InStates[BB]) {
            continue; // No change, skip recomputing OutState
        }

        InStates[BB] = std::move(NewInState);

        // 2b. Compute New OutState (Apply Transfer Functions)
        LockSet NewOutState = InStates[BB];

        for (Instruction &I : *BB) {
            NewOutState = runOnInstruction(I, NewOutState, Offset);
        }

        // Check if OutState changed
        if (NewOutState != OutStates[BB]) {
            OutStates[BB] = std::move(NewOutState);

            // Add successors to worklist since their InStates may change
            for (BasicBlock *Succ : successors(BB)) {
                AddToWorklist(Succ);
            }
        }
    }

    if (iterations >= MAX_ITERATIONS) {
        errs() << "Warning: Data flow analysis did not converge for function "
               << F.getName() << " after " << MAX_ITERATIONS << " iterations\n";
    }
}

// -----------------------------------------------------------
// 3. Main Pass
// -----------------------------------------------------------

PreservedAnalyses CriticalSectionTraversalPass::run(
    Module &M,
    ModuleAnalysisManager &AM)
{
    errs() << "=== Critical Section Analysis ===\n\n";

    size_t totalFunctions = 0;
    size_t totalInstructions = 0;
    size_t totalCriticalInstructions = 0;
    std::map<std::string, size_t> functionCriticalCounts;

    for (Function &F : M) {
        if (F.isDeclaration() || F.empty())
            continue;

        totalFunctions++;

        std::map<const BasicBlock*, LockSet> InStates;

        // Step 1: Run data flow analysis
        computeHeldLocks(F, InStates, OffsetValue);

        size_t functionInstructionCount = 0;
        size_t criticalSectionInstructionCount = 0;
        std::map<const Value*, size_t> lockUsage; // Track which locks protect most code

        // Track potential issues
        bool hasNestedLocks = false;
        size_t maxLockDepth = 0;

        // Step 2: Count instructions in critical sections
        for (BasicBlock &BB : F) {
            LockSet CurrentLocks = InStates.at(&BB);

            for (Instruction &I : BB) {
                functionInstructionCount++;

                // Count instruction if ANY lock is held
                if (!CurrentLocks.empty()) {
                    criticalSectionInstructionCount++;

                    // Track lock usage statistics
                    for (const Value* Lock : CurrentLocks) {
                        lockUsage[Lock]++;
                    }

                    // Track lock nesting depth
                    if (CurrentLocks.size() > 1) {
                        hasNestedLocks = true;
                    }
                    maxLockDepth = std::max(maxLockDepth, CurrentLocks.size());
                }

                // Update state for next instruction
                CurrentLocks = runOnInstruction(I, CurrentLocks, OffsetValue);
            }
        }

        totalInstructions += functionInstructionCount;
        totalCriticalInstructions += criticalSectionInstructionCount;

        // Report per-function statistics
        if (criticalSectionInstructionCount > 0 || !lockUsage.empty()) {
            errs() << "Function: " << F.getName() << "\n";
            errs() << "  Total Instructions: " << functionInstructionCount << "\n";
            errs() << "  Critical Section Instructions: " << criticalSectionInstructionCount;

            if (functionInstructionCount > 0) {
                double percentage = (100.0 * criticalSectionInstructionCount) / functionInstructionCount;
                errs() << " (" << format("%.2f", percentage) << "%)";
            }
            errs() << "\n";

            // Report lock nesting information
            if (hasNestedLocks) {
                errs() << "  WARNING: Nested locks detected (max depth: " << maxLockDepth << ")\n";
            }

            // Report lock usage if there are critical sections
            if (!lockUsage.empty()) {
                errs() << "  Locks used:\n";
                for (const auto& entry : lockUsage) {
                    errs() << "    ";
                    entry.first->printAsOperand(errs(), false);
                    errs() << ": protects " << entry.second << " instructions\n";
                }
            }
            errs() << "\n";

            functionCriticalCounts[F.getName().str()] = criticalSectionInstructionCount;
            }
    }

    // Summary statistics
    errs() << "=== Summary ===\n";
    errs() << "Total Functions Analyzed: " << totalFunctions << "\n";
    errs() << "Functions with Critical Sections: " << functionCriticalCounts.size() << "\n";
    errs() << "Total Instructions: " << totalInstructions << "\n";
    errs() << "Total Critical Section Instructions: " << totalCriticalInstructions;

    if (totalInstructions > 0) {
        double percentage = (100.0 * totalCriticalInstructions) / totalInstructions;
        errs() << " (" << format("%.2f", percentage) << "%)";
    }
    errs() << "\n\n";

    // Top functions by critical section size
    if (!functionCriticalCounts.empty()) {
        errs() << "Top Functions by Critical Section Size:\n";

        std::vector<std::pair<std::string, size_t>> sortedFunctions(
            functionCriticalCounts.begin(), functionCriticalCounts.end());

        std::sort(sortedFunctions.begin(), sortedFunctions.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        size_t topN = std::min(size_t(10), sortedFunctions.size());
        for (size_t i = 0; i < topN; ++i) {
            errs() << "  " << (i + 1) << ". " << sortedFunctions[i].first
                   << ": " << sortedFunctions[i].second << " instructions\n";
        }
        errs() << "\n";
    }

    errs().flush();
    return PreservedAnalyses::all();
}
