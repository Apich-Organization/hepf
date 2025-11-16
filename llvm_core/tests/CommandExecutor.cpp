#include "CommandExecutor.h"
#include <cstdlib>      // For std::system
#include <fstream>      // For std::ifstream
#include <sstream>      // For std::ostringstream
#include <cstdio>       // For std::remove (file cleanup)

namespace fs = std::filesystem;

// --- Private Helper Functions ---

bool CommandExecutor::execute_simple(const std::string& command_str) const {
    std::cout << "-> Running Command: " << command_str << std::endl;
    int result = std::system(command_str.c_str());
    return result == 0;
}

std::string CommandExecutor::read_file_content(const fs::path& file_path) const {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return ""; // Return empty string if file cannot be opened
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// --- Public Methods ---

CommandExecutor::CommandExecutor(const fs::path& project_root) :
    m_project_root(project_root) {
    std::cout << "Project Root set to: " << m_project_root.string() << std::endl;
}

// This method remains largely the same
bool CommandExecutor::run_compile_command(const std::string& source_file_name, const std::string& opt_level) {
    fs::path source_path = m_project_root / m_tests_subdir / source_file_name;

    std::string ll_name = source_file_name;
    ll_name.replace(ll_name.find(".cpp"), 4, ".ll");
    fs::path output_path = m_tmp_dir / ll_name;

    std::string opt_level_str = "-O" + opt_level;

    std::string command = "clang++ -S -emit-llvm "
                          + opt_level_str
                          + " -o "
                          + output_path.string() + " "
                          + source_path.string();

    return execute_simple(command);
}

CommandResult CommandExecutor::run_opt_command(
    const std::string& source_file_name,
    const std::string& pass_name,
    const std::string& so_name
) {
    // 1. Setup paths for command and logs
    std::string ll_name = source_file_name;
    ll_name.replace(ll_name.find(".cpp"), 4, ".ll");
    fs::path ll_input_path = m_tmp_dir / ll_name;
    fs::path so_path = m_project_root / m_build_subdir / so_name;

    fs::path stdout_log_path = m_tmp_dir / ("opt_" + ll_name + "_stdout.log");
    fs::path stderr_log_path = m_tmp_dir / ("opt_" + ll_name + "_stderr.log");

    // 2. Construct the base opt command
    std::string base_command = "opt -load-pass-plugin " + so_path.string()
                          + " -passes=" + pass_name
                          + " -disable-output " + ll_input_path.string();

    // 3. Construct the full command with output redirection
    std::string full_command = base_command
                               + " > " + stdout_log_path.string()
                               + " 2> " + stderr_log_path.string();

    std::cout << "-> Running Command: " << full_command << std::endl;

    // 4. Execute the command
    int ret_code = std::system(full_command.c_str());

    // 5. Read the captured output
    CommandResult result;
    result.return_code = ret_code;
    result.success = (ret_code == 0);
    result.stdout_output = read_file_content(stdout_log_path);
    result.stderr_output = read_file_content(stderr_log_path);

    // 6. Cleanup log files (Optional, but recommended for /tmp)
    std::remove(stdout_log_path.c_str());
    std::remove(stderr_log_path.c_str());

    return result;
}

// Function to execute a command and capture its output
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
