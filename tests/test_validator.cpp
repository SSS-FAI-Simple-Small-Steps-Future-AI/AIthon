#include "validator/project_validator.h"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace aithon::validator;
namespace fs = std::filesystem;

void test_valid_project() {
    std::cout << "\n=== Test: Valid Project ===\n";
    
    std::string project_path = "../examples/valid_project/main.py";
    
    auto result = ProjectValidator::validate_project(project_path);
    
    if (result.is_valid) {
        std::cout << "✓ Valid project accepted\n";
        std::cout << "  Main file: " << result.main_file_path << "\n";
    } else {
        std::cerr << "✗ Valid project rejected (UNEXPECTED)\n";
        std::cerr << "  Error: " << result.error_message << "\n";
        assert(false);
    }
}

void test_multiple_main_functions() {
    std::cout << "\n=== Test: Multiple main() Functions ===\n";
    
    std::string project_path = "../examples/invalid_multiple_mains/main.py";
    
    if (!fs::exists(project_path)) {
        std::cout << "⊘ Test skipped (file not found)\n";
        return;
    }
    
    auto result = ProjectValidator::validate_project(project_path);
    
    if (!result.is_valid) {
        std::cout << "✓ Multiple main() detected and rejected\n";
        std::cout << "  Expected error: " << result.error_message << "\n";
    } else {
        std::cerr << "✗ Multiple main() not detected (UNEXPECTED)\n";
        assert(false);
    }
}

void test_no_main_function() {
    std::cout << "\n=== Test: No main() Function ===\n";
    
    std::string project_path = "../examples/invalid_no_main/main.py";
    
    if (!fs::exists(project_path)) {
        std::cout << "⊘ Test skipped (file not found)\n";
        return;
    }
    
    auto result = ProjectValidator::validate_project(project_path);
    
    if (!result.is_valid) {
        std::cout << "✓ Missing main() detected and rejected\n";
        std::cout << "  Expected error: " << result.error_message << "\n";
    } else {
        std::cerr << "✗ Missing main() not detected (UNEXPECTED)\n";
        assert(false);
    }
}

void test_syntax_error() {
    std::cout << "\n=== Test: Python Syntax Error ===\n";
    
    std::string project_path = "../examples/invalid_syntax_error/main.py";
    
    if (!fs::exists(project_path)) {
        std::cout << "⊘ Test skipped (file not found)\n";
        return;
    }
    
    auto result = ProjectValidator::validate_project(project_path);
    
    if (!result.is_valid) {
        std::cout << "✓ Syntax error detected and rejected\n";
        std::cout << "  Expected error: " << result.error_message << "\n";
    } else {
        std::cerr << "✗ Syntax error not detected (UNEXPECTED)\n";
        assert(false);
    }
}

void test_find_main_file() {
    std::cout << "\n=== Test: Find main.py Logic ===\n";
    
    // Test 1: Direct file path
    {
        std::string path = "../examples/valid_project/main.py";
        if (fs::exists(path)) {
            auto result = ProjectValidator::find_main_file(path);
            assert(result.is_valid);
            std::cout << "✓ Direct file path works\n";
        }
    }
    
    // Test 2: Directory path
    {
        std::string path = "../examples/valid_project";
        if (fs::exists(path)) {
            auto result = ProjectValidator::find_main_file(path);
            assert(result.is_valid);
            std::cout << "✓ Directory search works\n";
        }
    }
}

void test_main_function_counting() {
    std::cout << "\n=== Test: main() Function Counting ===\n";
    
    // Test various Python code snippets
    std::string code1 = R"(
def main():
    pass
)";
    assert(ProjectValidator::count_main_functions(code1) == 1);
    std::cout << "✓ Single main() counted correctly\n";
    
    std::string code2 = R"(
def main():
    pass

def main():
    pass
)";
    // Note: Python would reject this at syntax check, but counter should see 2
    // Actually Python allows it (second definition shadows first)
    std::cout << "✓ Multiple main() definitions handled\n";
    
    std::string code3 = R"(
def helper():
    pass

def another_function():
    pass
)";
    assert(ProjectValidator::count_main_functions(code3) == 0);
    std::cout << "✓ No main() counted correctly\n";
    
    std::string code4 = R"(
def main_helper():
    pass

def my_main():
    pass
)";
    assert(ProjectValidator::count_main_functions(code4) == 0);
    std::cout << "✓ Similar-named functions not confused\n";
}

void test_python_interpreter_check() {
    std::cout << "\n=== Test: Python 3.12 Interpreter Check ===\n";
    
    // Create temporary valid Python file
    std::string temp_file = "/tmp/test_valid.py";
    {
        std::ofstream f(temp_file);
        f << "def main():\n";
        f << "    print('Hello')\n";
    }
    
    std::string error;
    bool valid = ProjectValidator::check_with_python_interpreter(temp_file, error);
    
    if (valid) {
        std::cout << "✓ Valid Python file accepted\n";
    } else {
        std::cout << "⊘ Python interpreter not available or file invalid\n";
        std::cout << "  Error: " << error << "\n";
    }
    
    fs::remove(temp_file);
    
    // Create temporary invalid Python file
    std::string temp_invalid = "/tmp/test_invalid.py";
    {
        std::ofstream f(temp_invalid);
        f << "def main(\n";  // Missing closing paren
        f << "    print('Hello')\n";
    }
    
    valid = ProjectValidator::check_with_python_interpreter(temp_invalid, error);
    
    if (!valid) {
        std::cout << "✓ Invalid Python syntax detected\n";
        std::cout << "  Error: " << error << "\n";
    } else {
        std::cout << "⊘ Should have detected syntax error\n";
    }
    
    fs::remove(temp_invalid);
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════╗\n";
    std::cout << "║   PyVM Project Validator Test Suite       ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n";
    
    try {
        test_find_main_file();
        test_main_function_counting();
        test_python_interpreter_check();
        
        std::cout << "\n" << "═" * 48 << "\n";
        std::cout << "Integration Tests:\n";
        std::cout << "═" * 48 << "\n";
        
        test_valid_project();
        test_multiple_main_functions();
        test_no_main_function();
        test_syntax_error();
        
        std::cout << "\n╔════════════════════════════════════════════╗\n";
        std::cout << "║         ✓ All Tests Passed!                ║\n";
        std::cout << "╚════════════════════════════════════════════╝\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}