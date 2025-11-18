#include "CommandExecutor.h"
#include "gtest/gtest.h"
#include <fstream>
#include <iostream>
#include <string>

TEST(PathBasedCriticalSectionTraversalTest,
     CorrectlyCountsPathBasedCriticalSections) {
  CommandExecutor executor(PROJECT_ROOT_PATH);

  std::string test_file = "test_path_based_critical_section_traversal.cpp";
  std::string pass_name = "path-based-critical-section-traversal";
  std::string opt_level = "0";

  // 1. Compile the test file to LLVM IR
  executor.run_compile_command(test_file, opt_level);

  // Print the IR
  std::cout << "--- LLVM IR ---" << std::endl;
  std::ifstream ir_file("/tmp/test_path_based_critical_section_traversal.ll");
  std::string ir_line;
  while (std::getline(ir_file, ir_line)) {
    std::cout << ir_line << std::endl;
  }
  std::cout << "---------------" << std::endl;

  // 2. Run the opt command with our pass
  CommandResult opt_result = executor.run_opt_command(test_file, pass_name);

  if (opt_result.success) {
    std::cout << "\nOPT Pass Executed Successfully!" << std::endl;
    std::cout << "--- STDOUT ---\n" << opt_result.stdout_output;
    std::cout << "--- STDERR ---\n" << opt_result.stderr_output;

    // Example: Check for a specific output string
    if (opt_result.stdout_output.find("Expected result pattern found") !=
        std::string::npos) {
      std::cout << "\nTest PASSED: Expected output confirmed." << std::endl;
    }
  } else {
    std::cerr << "\nOPT Pass FAILED! Return Code: " << opt_result.return_code
              << std::endl;
    std::cerr << "--- STDERR ---\n" << opt_result.stderr_output;
  }

  const std::string &stderr_output = opt_result.stderr_output;
  const std::string &stdcout_output = opt_result.stdout_output;

  // 3. Check the output
  ASSERT_TRUE(opt_result.success);
  ASSERT_TRUE(
      opt_result.stderr_output.find("Function: _Z15locked_functionv, Path (BBs): [%0], Final Lock Depth: 0") !=
      std::string::npos);
  ASSERT_TRUE(opt_result.stderr_output.find("Function: _Z11mixed_locksv, Path (BBs): [%0], Final Lock Depth: 0") !=
              std::string::npos);
}
