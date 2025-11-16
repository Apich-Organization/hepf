find_package(GTest REQUIRED)

# Get the absolute path to your project source directory
get_filename_component(HEPF_CPP_CORE_ROOT ${CMAKE_SOURCE_DIR} ABSOLUTE)

# Add the test executable
add_executable(run_tests
  main_maxpath.cpp
  main_fanout.cpp
  main_critical_section.cpp
  main_critical_section_traversal.cpp
  CommandExecutor.cpp
)

# Use target_compile_definitions to make the path available in C++ code
target_compile_definitions(run_tests PRIVATE
    PROJECT_ROOT_PATH="${HEPF_CPP_CORE_ROOT}"
)

# Set public include directory for the library tests
target_include_directories(run_tests PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Link against GTest
target_link_libraries(run_tests PRIVATE GTest::gtest_main)

add_test(NAME run_tests COMMAND run_tests)
