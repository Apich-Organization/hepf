#include "cffi.h"
#include "generic_ffi_wrappers.h"
#include <iostream>

// LLVM Namespace Passes
IMPLEMENT_CPPASS_FFI(llvm, CriticalSectionTraversalPass)
IMPLEMENT_CPPASS_FFI(llvm, CriticalSectionTraversalAlternativePass)

// HEPF Namespace Passes
IMPLEMENT_CPPASS_FFI(hepf, FeedbackResonance)
IMPLEMENT_CPPASS_FFI(hepf, FlowDensity)
IMPLEMENT_CPPASS_FFI(llvm, InterProcFanOutPass)
IMPLEMENT_CPPASS_FFI(llvm, MaxPathPass)
IMPLEMENT_CPPASS_FFI(hepf, PathBasedCriticalSectionTraversalPass)
IMPLEMENT_CPPASS_FFI(hepf, PathBasedFeedbackResonancePass)
IMPLEMENT_CPPASS_FFI(hepf, PathBasedFlowDensityPass)
IMPLEMENT_CPPASS_FFI(hepf, PathBasedInterProcFanOutPass)
IMPLEMENT_CPPASS_FFI(hepf, PathBasedMaxPathPass)
IMPLEMENT_CPPASS_FFI(hepf, PathEnumeratorPass)
