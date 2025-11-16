cmake_minimum_required(VERSION 3.16)
project(MyProjectName LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- LLVM Configuration ---
find_package(LLVM REQUIRED CONFIG)

# Add global LLVM include directories and definitions
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# --- Target Definition ---
# Build the pass as a module (dynamically loadable plugin)
# This avoids linking LLVM libraries into the plugin itself.
add_library(hepf_core MODULE
    src/MaxPath.cpp
    src/PassPlugin.cpp
    src/InterProcFanOut.cpp
    src/CriticalSectionTraversal.cpp
    src/CriticalSection.cpp
    include/MaxPath.h
    include/InterProcFanOut.h
    include/CriticalSectionTraversal.h
    include/CriticalSection.h
    include/hepf.h
)

# Set public include directory for the library
target_include_directories(hepf_core PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# --- Testing ---
enable_testing()
add_subdirectory(tests)
