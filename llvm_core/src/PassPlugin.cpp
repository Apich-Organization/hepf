#include "MaxPath.h"
#include "InterProcFanOut.h"
#include "CriticalSectionTraversal.h"
#include "CriticalSection.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

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
                    // Change the offset to a different value before testing different conditions
                    std::int64_t Offset = 0;
                    MPM.addPass(CriticalSectionTraversalPass(Offset));
                    return true;
                  }
                  return false;
                });
          }};
}
