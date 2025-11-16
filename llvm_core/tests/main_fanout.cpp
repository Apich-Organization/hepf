#include "gtest/gtest.h"
#include <iostream>
#include <fstream>
#include <string>
#include "CommandExecutor.h"

TEST(InterProcFanOutPassTest, CorrectlyCalculatesFanOut) {
    CommandExecutor executor(PROJECT_ROOT_PATH);

    std::string test_file = "test_fanout.cpp";
    std::string pass_name = "inter-proc-fan-out";
    std::string opt_level = "0";

    // 1. Compile the test file to LLVM IR
    executor.run_compile_command(test_file, opt_level);

    // Print the IR
    std::cout << "--- LLVM IR ---" << std::endl;
    std::ifstream ir_file("/tmp/test_critical_section.ll");
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
            if (opt_result.stdout_output.find("Expected result pattern found") != std::string::npos) {
                std::cout << "\nTest PASSED: Expected output confirmed." << std::endl;
            }
        } else {
            std::cerr << "\nOPT Pass FAILED! Return Code: " << opt_result.return_code << std::endl;
            std::cerr << "--- STDERR ---\n" << opt_result.stderr_output;
        }

    const std::string& stderr_output = opt_result.stderr_output;
    const std::string& stdcout_output = opt_result.stdout_output;

    // 3. Check the output
    // Note: The function names are mangled.
    ASSERT_TRUE(stderr_output.find("Function: _Z1aPi, Fan-out: 0") != std::string::npos);
    ASSERT_TRUE(stderr_output.find("Function: _Z1bv, Fan-out: 1") != std::string::npos);
    ASSERT_TRUE(stderr_output.find("Function: _Z1cv, Fan-out: 1") != std::string::npos);
    ASSERT_TRUE(stderr_output.find("Function: _Z1dv, Fan-out: 0") != std::string::npos);
    ASSERT_TRUE(stderr_output.find("Function: _Z1ev, Fan-out: 2") != std::string::npos);
}
