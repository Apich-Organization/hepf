cmake_minimum_required(VERSION 3.16)
project(hepf_core LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Position Independent Code
set(CMAKE_POSITION_INDEPENDENT_CODE ON FORCE)
# LTO enabled
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE FORCE)
# Flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "Flags used by the CXX compiler for release builds." FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-O3 -DNDEBUG" CACHE STRING "Flags used by the CXX compiler for debug builds." FORCE)

# --- LLVM Configuration ---
find_package(LLVM REQUIRED CONFIG)

# Add global LLVM include directories and definitions
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# --- Target Definition ---
# Build the pass as a object library
add_library(hepf_core_obj OBJECT
    include/MaxPath.h
    include/InterProcFanOut.h
    include/CriticalSectionTraversal.h
    include/CriticalSection.h
    include/FlowDensity.h
    include/FeedbackResonance.h
    include/PathEnumerator.h
    include/PathEnumeratorPass.h
    include/PathBasedMaxPath.h
    include/PathBasedInterProcFanOut.h
    include/PathBasedCriticalSectionTraversal.h
    include/PathBasedFlowDensity.h
    include/PathBasedFeedbackResonance.h
    include/hepf.h
    include/generic_ffi_wrappers.h
    include/cffi.h
    src/MaxPath.cpp
    src/PassPlugin.cpp
    src/InterProcFanOut.cpp
    src/CriticalSectionTraversal.cpp
    src/CriticalSection.cpp
    src/FlowDensity.cpp
    src/FeedbackResonance.cpp
    src/PathEnumerator.cpp
    src/PathEnumeratorPass.cpp
    src/PathBasedMaxPath.cpp
    src/PathBasedInterProcFanOut.cpp
    src/PathBasedCriticalSectionTraversal.cpp
    src/PathBasedFlowDensity.cpp
    src/PathBasedFeedbackResonance.cpp
    src/cffi.cpp
)

set_target_properties(hepf_core_obj PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Build the pass as a module (dynamically loadable plugin)
# This avoids linking LLVM libraries into the plugin itself.
add_library(hepf_core_module MODULE
    $<TARGET_OBJECTS:hepf_core_obj>
)

# Build the pass as a static
add_library(hepf_core_static STATIC
    $<TARGET_OBJECTS:hepf_core_obj>
)

# Build the pass as a shared library
add_library(hepf_core_shared SHARED
    $<TARGET_OBJECTS:hepf_core_obj>
)

# Set public include directory for the library
target_include_directories(hepf_core_obj PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(hepf_core_shared PRIVATE ${LLVM_LIBS})

# Set public include directory for the library
target_include_directories(hepf_core_shared PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(hepf_core_module PRIVATE ${LLVM_LIBS})

# Set public include directory for the library
target_include_directories(hepf_core_module PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(hepf_core_static PRIVATE ${LLVM_LIBS})

# Set public include directory for the library
target_include_directories(hepf_core_static PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# --- Testing ---
enable_testing()
add_subdirectory(tests)
