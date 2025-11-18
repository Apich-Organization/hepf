#include "llvm/Passes/PassPlugin.h"
#include "CriticalSection.h"
#include "CriticalSectionTraversal.h"
#include "FeedbackResonance.h"
#include "FlowDensity.h"
#include "InterProcFanOut.h"
#include "MaxPath.h"
#include "PathBasedCriticalSectionTraversal.h"
#include "PathBasedFeedbackResonance.h"
#include "PathBasedFlowDensity.h"
#include "PathBasedInterProcFanOut.h"
#include "PathBasedMaxPath.h"
#include "PathEnumeratorPass.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HepfCore", "v0.1.0", [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "max-path") {
                    MPM.addPass(MaxPathPass());
                    return true;
                  }
                  if (Name == "inter-proc-fan-out") {
                    MPM.addPass(InterProcFanOutPass());
                    return true;
                  }
                  if (Name == "critical-section-traversal") {
                    MPM.addPass(CriticalSectionTraversalAlternativePass());
                    return true;
                  }
                  if (Name == "critical-section") {
                    // Change the offset to a different value before testing
                    // different conditions
                    std::int64_t Offset = 0;
                    MPM.addPass(CriticalSectionTraversalPass());
                    return true;
                  }
                  if (Name == "hepf-flow-density") {
                    MPM.addPass(hepf::FlowDensity());
                    return true;
                  }
                  if (Name == "hepf-feedback-resonance") {
                    MPM.addPass(hepf::FeedbackResonance());
                    return true;
                  }
                  if (Name == "path-enumerator") {
                    MPM.addPass(hepf::PathEnumeratorPass(1000, 2));
                    return true;
                  }
                  if (Name == "path-based-max-path") {
                    MPM.addPass(hepf::PathBasedMaxPathPass());
                    return true;
                  }
                  if (Name == "path-based-inter-proc-fan-out") {
                    MPM.addPass(hepf::PathBasedInterProcFanOutPass());
                    return true;
                  }
                  if (Name == "path-based-critical-section-traversal") {
                    MPM.addPass(hepf::PathBasedCriticalSectionTraversalPass(10000, 2));
                    return true;
                  }
                  if (Name == "path-based-flow-density") {
                    MPM.addPass(hepf::PathBasedFlowDensityPass(1000, 2));
                    return true;
                  }
                  if (Name == "path-based-feedback-resonance") {
                    // Change the offset to a different value before testing
                    // different conditions
                    float DefaultEntropyThreshold = 10.0f;
                    MPM.addPass(hepf::PathBasedFeedbackResonancePass(DefaultEntropyThreshold));
                    return true;
                  }
                  return false;
                });
          }};
}
