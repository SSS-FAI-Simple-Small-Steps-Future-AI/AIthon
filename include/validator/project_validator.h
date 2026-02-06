#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace pyvm::validator {

    struct ValidationResult {
        bool is_valid;
        std::string error_message;
        std::string main_file_path;

        ValidationResult() : is_valid(false) {}
        ValidationResult(bool valid, const std::string& error = "", const std::string& path = "")
            : is_valid(valid), error_message(error), main_file_path(path) {}
    };

    class ProjectValidator {
    public:
        // Validate project structure and requirements
        static ValidationResult validate_project(const std::string& project_path);

        // Check for exactly one main.py file
        static ValidationResult find_main_file(const std::string& project_path);

        // Validate main.py has exactly one main() function
        static ValidationResult validate_main_function(const std::string& main_file_path);

        // Validate Python syntax using Python 3.12 interpreter
        static ValidationResult validate_python_syntax(const std::string& file_path);

        // Run all validations in sequence
        static ValidationResult run_all_validations(const std::string& project_path);

    private:
        // Helper to count main.py files recursively
        static std::vector<std::string> find_all_main_files(const std::string& directory);

        // Helper to count main() function definitions
        static int count_main_functions(const std::string& file_content);

        // Execute Python 3.12 to check syntax
        static bool check_with_python_interpreter(const std::string& file_path,
                                                  std::string& error_output);
    };

} // namespace pyvm::validator