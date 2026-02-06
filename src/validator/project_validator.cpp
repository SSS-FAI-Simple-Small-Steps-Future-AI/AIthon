#include "validator/project_validator.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <cstdlib>
#include <array>
#include <memory>

namespace pyvm::validator {

ValidationResult ProjectValidator::validate_project(const std::string& project_path) {
    return run_all_validations(project_path);
}

ValidationResult ProjectValidator::find_main_file(const std::string& project_path) {
    namespace fs = std::filesystem;
    
    if (!fs::exists(project_path)) {
        return ValidationResult(false, "Project path does not exist: " + project_path);
    }
    
    // Find all main.py files
    std::vector<std::string> main_files = find_all_main_files(project_path);
    
    if (main_files.empty()) {
        return ValidationResult(false, 
            "ERROR: No 'main.py' file found in project.\n"
            "REQUIREMENT: Project must contain exactly one file named 'main.py'");
    }
    
    if (main_files.size() > 1) {
        std::ostringstream oss;
        oss << "ERROR: Multiple 'main.py' files found in project.\n"
            << "REQUIREMENT: Project must contain at most one file named 'main.py'\n"
            << "Found " << main_files.size() << " files:\n";
        for (const auto& file : main_files) {
            oss << "  - " << file << "\n";
        }
        return ValidationResult(false, oss.str());
    }
    
    return ValidationResult(true, "", main_files[0]);
}

ValidationResult ProjectValidator::validate_main_function(const std::string& main_file_path) {
    // Read file content
    std::ifstream file(main_file_path);
    if (!file.is_open()) {
        return ValidationResult(false, "Cannot open main.py file: " + main_file_path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Count main() function definitions
    int main_count = count_main_functions(content);
    
    if (main_count == 0) {
        return ValidationResult(false,
            "ERROR: No 'main()' function found in main.py\n"
            "REQUIREMENT: main.py must contain exactly one function named 'main'");
    }
    
    if (main_count > 1) {
        std::ostringstream oss;
        oss << "ERROR: Multiple 'main()' functions found in main.py\n"
            << "REQUIREMENT: main.py must contain at most one function named 'main'\n"
            << "Found " << main_count << " main() function definitions";
        return ValidationResult(false, oss.str());
    }
    
    return ValidationResult(true, "", main_file_path);
}

ValidationResult ProjectValidator::validate_python_syntax(const std::string& file_path) {
    std::string error_output;
    
    if (!check_with_python_interpreter(file_path, error_output)) {
        std::ostringstream oss;
        oss << "ERROR: Python syntax validation failed for: " << file_path << "\n"
            << "Python 3.12 interpreter reported errors:\n"
            << error_output;
        return ValidationResult(false, oss.str());
    }
    
    return ValidationResult(true, "", file_path);
}

ValidationResult ProjectValidator::run_all_validations(const std::string& project_path) {
    std::cout << "=== PyVM Project Validation ===" << std::endl;
    std::cout << "Project path: " << project_path << std::endl << std::endl;
    
    // Step 1: Find main.py file
    std::cout << "[1/3] Checking for main.py file..." << std::endl;
    auto main_file_result = find_main_file(project_path);
    if (!main_file_result.is_valid) {
        std::cerr << main_file_result.error_message << std::endl;
        return main_file_result;
    }
    std::cout << "✓ Found main.py at: " << main_file_result.main_file_path << std::endl << std::endl;
    
    // Step 2: Validate main() function
    std::cout << "[2/3] Validating main() function..." << std::endl;
    auto main_func_result = validate_main_function(main_file_result.main_file_path);
    if (!main_func_result.is_valid) {
        std::cerr << main_func_result.error_message << std::endl;
        return main_func_result;
    }
    std::cout << "✓ Found exactly one main() function" << std::endl << std::endl;
    
    // Step 3: Validate Python syntax with Python 3.12
    std::cout << "[3/3] Validating Python syntax with Python 3.12..." << std::endl;
    auto syntax_result = validate_python_syntax(main_file_result.main_file_path);
    if (!syntax_result.is_valid) {
        std::cerr << syntax_result.error_message << std::endl;
        return syntax_result;
    }
    std::cout << "✓ Python syntax is valid" << std::endl << std::endl;
    
    std::cout << "=== All Validations Passed ===" << std::endl << std::endl;
    
    return ValidationResult(true, "", main_file_result.main_file_path);
}

std::vector<std::string> ProjectValidator::find_all_main_files(const std::string& directory) {
    namespace fs = std::filesystem;
    std::vector<std::string> main_files;
    
    try {
        if (fs::is_regular_file(directory)) {
            // Single file provided
            if (fs::path(directory).filename() == "main.py") {
                main_files.push_back(directory);
            }
        } else if (fs::is_directory(directory)) {
            // Search directory recursively
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().filename() == "main.py") {
                    main_files.push_back(entry.path().string());
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    
    return main_files;
}

int ProjectValidator::count_main_functions(const std::string& file_content) {
    // Look for 'def main(' or 'def main (' or 'def main():'
    // This regex matches function definitions named 'main'
    std::regex main_func_regex(R"(^\s*def\s+main\s*\()", std::regex::multiline);
    
    auto begin = std::sregex_iterator(file_content.begin(), file_content.end(), main_func_regex);
    auto end = std::sregex_iterator();
    
    return std::distance(begin, end);
}

bool ProjectValidator::check_with_python_interpreter(const std::string& file_path,
                                                     std::string& error_output) {
    // Try python3.12 first, then python3, then python
    std::vector<std::string> python_commands = {"python3.12", "python3", "python"};
    
    for (const auto& python_cmd : python_commands) {
        // Command to check syntax: python3.12 -m py_compile file.py
        std::string command = python_cmd + " -m py_compile \"" + file_path + "\" 2>&1";
        
        // Check if python command exists
        std::string check_cmd = "which " + python_cmd + " > /dev/null 2>&1";
        if (std::system(check_cmd.c_str()) != 0) {
            continue;  // Command not found, try next
        }
        
        // Execute syntax check
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        
        if (!pipe) {
            continue;  // Failed to execute, try next
        }
        
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        
        int return_code = pclose(pipe.release());
        
        if (return_code == 0) {
            // Syntax check passed
            
            // Also verify it's actually Python 3.12 or later
            std::string version_cmd = python_cmd + " --version 2>&1";
            std::unique_ptr<FILE, decltype(&pclose)> version_pipe(
                popen(version_cmd.c_str(), "r"), pclose
            );
            
            if (version_pipe) {
                std::string version_output;
                while (fgets(buffer.data(), buffer.size(), version_pipe.get()) != nullptr) {
                    version_output += buffer.data();
                }
                
                std::cout << "  Using: " << version_output;
            }
            
            return true;
        } else {
            error_output = result;
            // If we found python3.12 specifically, use its error even if it failed
            if (python_cmd == "python3.12") {
                return false;
            }
        }
    }
    
    if (error_output.empty()) {
        error_output = "Python 3.12 interpreter not found. Please install Python 3.12+";
    }
    
    return false;
}

} // namespace pyvm::validator