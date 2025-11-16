#ifndef COMMAND_EXECUTOR_H
#define COMMAND_EXECUTOR_H

#include <string>
#include <filesystem>
#include <iostream>

// Structure to hold the results of an executed command
struct CommandResult {
    bool success;
    int return_code;
    std::string stdout_output;
    std::string stderr_output;
};

class CommandExecutor {
public:
    CommandExecutor(const std::filesystem::path& project_root);

    // Renaming the compile command to reflect the output path is fixed
    bool run_compile_command(const std::string& source_file_name, const std::string& opt_level);

    /**
     * @brief Constructs and runs the opt pass command, capturing stdout/stderr.
     * @param source_file_name The name of the source file (to deduce the .ll file).
     * @param pass_name The name of the optimization pass.
     * @param so_name The name of the shared object/plugin file.
     * @return CommandResult containing success status, return code, and captured output.
     */
    CommandResult run_opt_command(
        const std::string& source_file_name,
        const std::string& pass_name,
        const std::string& so_name = "libhepf_core.so"
    );

private:
    std::filesystem::path m_project_root;
    const std::filesystem::path m_tmp_dir = "/tmp";
    const std::filesystem::path m_tests_subdir = "tests";
    const std::filesystem::path m_build_subdir = "build";

    // Renamed the execute helper to be more specific to system calls without capture
    bool execute_simple(const std::string& command_str) const;

    // Helper to read the content of a file into a string
    std::string read_file_content(const std::filesystem::path& file_path) const;
};

#endif // COMMAND_EXECUTOR_H
