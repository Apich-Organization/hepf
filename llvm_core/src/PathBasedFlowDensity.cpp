#include "PathBasedFlowDensity.h"
#include "PathEnumerator.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace hepf;

// -----------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------
static double getEdgeProbability(BasicBlock *Src, BasicBlock *Dst,
                                 BranchProbabilityInfo &BPI) {
  auto *Term = Src->getTerminator();
  if (!Term || Term->getNumSuccessors() == 0)
    return 0.0;

  for (unsigned i = 0, e = Term->getNumSuccessors(); i < e; ++i) {
    if (Term->getSuccessor(i) == Dst) {
      BranchProbability BP = BPI.getEdgeProbability(Src, i);
      if (BP.isUnknown())
        return 0.0;
      return static_cast<double>(BP.getNumerator()) / BP.getDenominator();
    }
  }
  return 0.0;
}

// -----------------------------------------------------------
// Main Pass
// -----------------------------------------------------------
PathBasedFlowDensityPass::PathBasedFlowDensityPass(
    size_t maxPaths,
    size_t maxLoopIterations)
    : MaxPaths(maxPaths),
      MaxLoopIterations(maxLoopIterations)
{
    // Definition of the constructor.
}

PreservedAnalyses PathBasedFlowDensityPass::run(Module &M,
                                                ModuleAnalysisManager &MAM) {
  for (Function &F : M) {
    if (F.isDeclaration() || F.empty())
      continue;

    // Correct way to get the FunctionAnalysisManager in the new PM
    auto &FAM =
        MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    // Required analysis
    auto &BPI = FAM.getResult<BranchProbabilityAnalysis>(F);

    runOnFunction(F, BPI);
  }

  return PreservedAnalyses::all();
}

// ------------------------------------------------------------
// Per-function implementation (now properly declared in the class)
void PathBasedFlowDensityPass::runOnFunction(Function &F,
                                             BranchProbabilityInfo &BPI) {
  PathEnumerator PE(F, MaxPaths, MaxLoopIterations);

  // Dummy entropy = number of instructions (replace with real entropy if
  // desired)
  DenseMap<BasicBlock *, float> bbEntropy;
  for (BasicBlock &BB : F)
    bbEntropy[&BB] = static_cast<float>(BB.size());

  errs() << "=== Path-Based Flow Density for '" << F.getName() << "' ===\n";

  for (const auto &Path : PE.getPaths()) {
    if (Path.empty())
      continue;

    double pathProb = 1.0;
    float pathEntropy = 0.0f;

    // Sum entropy of all blocks in the path
    for (BasicBlock *BB : Path)
      pathEntropy += bbEntropy.lookup(BB);

    // Multiply edge probabilities
    for (size_t i = 0; i + 1 < Path.size(); ++i) {
      double p = getEdgeProbability(Path[i], Path[i + 1], BPI);
      if (p <= 0.0) { // unknown or zero probability → skip path
        pathProb = 0.0;
        break;
      }
      pathProb *= p;
    }

    double flowDensity = pathProb * pathEntropy;

    errs() << "  Prob: " << format("%.6f", pathProb)
           << " | Entropy: " << format("%.2f", pathEntropy)
           << " | FlowDensity: " << format("%.6e", flowDensity) << "\n";

    errs() << "    Path: [";
    for (size_t i = 0; i < Path.size(); ++i) {
      BasicBlock *BB = Path[i];
      if (BB->hasName())
        errs() << BB->getName();
      else
        errs() << "<unnamed>";
      if (i + 1 < Path.size())
        errs() << " → ";
    }
    errs() << "]\n";
  }
  errs() << "\n";
}
