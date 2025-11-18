#ifndef CFFI_H
#define CFFI_H

// Include the generic FFI definitions
#include "generic_ffi_wrappers.h"
#include "CriticalSection.h" // Defines llvm::CriticalSectionTraversalPass
#include "CriticalSectionTraversal.h" // Defines llvm::CriticalSectionTraversalAlternativePass
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
#include "hepf.h" // For hepf namespace

#ifdef __cplusplus
extern "C" {
#endif

// LLVM Namespace Passes
DECLARE_CPPASS_FFI(llvm, CriticalSectionTraversalPass)
DECLARE_CPPASS_FFI(llvm, CriticalSectionTraversalAlternativePass)

// HEPF Namespace Passes
DECLARE_CPPASS_FFI(hepf, FeedbackResonance)
DECLARE_CPPASS_FFI(hepf, FlowDensity)
DECLARE_CPPASS_FFI(llvm, InterProcFanOutPass)
DECLARE_CPPASS_FFI(llvm, MaxPathPass)
DECLARE_CPPASS_FFI(hepf, PathBasedCriticalSectionTraversalPass)
DECLARE_CPPASS_FFI(hepf, PathBasedFeedbackResonancePass)
DECLARE_CPPASS_FFI(hepf, PathBasedFlowDensityPass)
DECLARE_CPPASS_FFI(hepf, PathBasedInterProcFanOutPass)
DECLARE_CPPASS_FFI(hepf, PathBasedMaxPathPass)
DECLARE_CPPASS_FFI(hepf, PathEnumeratorPass)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CFFI_H