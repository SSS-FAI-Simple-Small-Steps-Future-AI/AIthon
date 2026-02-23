//
// Created by Furqan on 12/02/2026.
//#include "../../include/compiler/compiler.h"

#include <fstream>

#include "../../include/validator/project_validator.h"
#include "../../include/validator/syntax_validator.h"
#include "../../include/syntax/syntax_preprocessor.h"
#include "../../include/ast/python_ast_converter.h"
#include "../../include/codegen/llvm_codegen.h"
#include <iostream>


namespace aithon::compiler {

Compiler::Compiler() = default;

Compiler::~Compiler() = default;

// Add this helper function at the top of your file or in a utilities file
std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


bool Compiler::compile_file(const std::string& input_file,
                            const std::string& output_file) {
    try {
        // ========================================
        // STEP 1: Validate Project Structure
        // ========================================
        std::cout << "╔════════════════════════════════════════════╗\n";
        std::cout << "║   AIthon Compiler - Strict Validation Mode   ║\n";
        std::cout << "╚════════════════════════════════════════════╝\n\n";

        auto project_validation_result = validator::ProjectValidator::run_all_validations(input_file);

        if (!project_validation_result.is_valid) {
            std::cerr << "\n❌ COMPILATION STOPPED: Validation failed\n";
            std::cerr << project_validation_result.error_message << std::endl;
            return false;
        }

        // Read source code
        std::string source_code = read_file(input_file);
        if (source_code.empty()) {
            std::cerr << "❌ Error: Could not read file or file is empty: " << input_file << std::endl;
            return false;
        }

        auto syntax_validation_result = validator::SyntaxValidator::validate(source_code);
        if (!syntax_validation_result.is_valid) {
            std::cerr << "\n❌ COMPILATION STOPPED: Syntax Validation failed\n";
            std::cerr << syntax_validation_result.errors << std::endl;
            return false;
        }

        std::cout << "✓ Syntax validation passed\n\n";

        // ========================================
        // STEP 4: Syntax Preprocessing (func → def, {} → :)
        // ========================================
        std::cout << "[3/6] Preprocessing syntax (func → def, {} → indentation)..." << std::endl;

        auto transform_result = syntax::SyntaxPreprocessor::transform(source_code);

        if (!transform_result.success) {
            std::cerr << "❌ Syntax transformation failed:\n";
            std::cerr << transform_result.error_message << std::endl;

            // Show transformed code for debugging
            if (!transform_result.transformed_code.empty()) {
                std::cerr << "\n--- Transformed code (for debugging) ---\n";
                std::cerr << transform_result.transformed_code << std::endl;
                std::cerr << "--- End transformed code ---\n";
            }

            return false;
        }

        std::cout << "✓ Syntax transformed to Python\n\n";

        // Optionally show transformed code
        if (getenv("PYVM_SHOW_TRANSFORMED")) {
            std::cout << "--- Transformed Python Code ---\n";
            std::cout << transform_result.transformed_code << std::endl;
            std::cout << "--- End ---\n\n";
        }

        // Write transformed code to temporary file for Python parser
        std::string temp_file = "/tmp/main" + std::to_string(time(nullptr)) + ".py";
        std::ofstream temp_out(temp_file);
        temp_out << transform_result.transformed_code;
        temp_out.close();


        // Step 3: Validate Python syntax with Python 3.12
        std::cout << "[3/3] Validating Python syntax with Python 3.12..." << std::endl;
        auto syntax_result = validator::ProjectValidator::validate_python_syntax(temp_file);
        if (!syntax_result.is_valid) {
            std::cerr << syntax_result.error_message << std::endl;
            return false;
        }

        std::cout << "✓ Python syntax is valid" << std::endl << std::endl;

        // std::string main_file = project_validation_result.main_file_path;
        std::string main_file = temp_file;


        // ========================================
        // STEP 2: Parse Python 3.12 Code to AST
        // ========================================
        std::cout << "=== Compilation Pipeline ===" << std::endl;
        std::cout << "[1/4] Parsing Python code to AST..." << std::endl;
        ast::PythonASTConverter converter;
        const auto module = converter.parse_file(main_file);
        std::cout << "✓ AST generated successfully\n\n";


        // Clean up temp file
        std::remove(temp_file.c_str());

        // ========================================
        // STEP 3: Generate LLVM IR from AST
        // ========================================
        std::cout << "[2/4] Generating LLVM IR..." << std::endl;
        codegen::LLVMCodeGen codegen("aithon_module");
        codegen.codegen(module.get());
        std::cout << "✓ LLVM IR generated\n\n";

        // ========================================
        // STEP 4: Optimize LLVM IR
        // ========================================
        std::cout << "[3/4] Running LLVM optimization passes..." << std::endl;
        codegen.optimize();
        std::cout << "✓ IR optimized\n\n";

        // ========================================
        // STEP 5: Generate Machine Code
        // ========================================
        std::cout << "[4/4] Generating optimized machine code..." << std::endl;
        const std::string object_file = output_file + ".o";
        codegen.emit_object_file(object_file);
        std::cout << "✓ Object file created: " << object_file << "\n\n";

        // Link with runtime
        std::cout << "Linking with AIthon runtime..." << std::endl;
        bool linked = link_with_runtime(object_file, output_file);

        if (linked) {
            std::cout << "\n╔════════════════════════════════════════════╗\n";
            std::cout << "║        ✓ COMPILATION SUCCESSFUL!           ║\n";
            std::cout << "╚════════════════════════════════════════════╝\n\n";
            std::cout << "Executable: " << output_file << "\n";
            std::cout << "\nRuntime Features:\n";
            std::cout << "  • Green Threads (M:N threading)\n";
            std::cout << "  • Actor-based Concurrency\n";
            std::cout << "  • Per-Thread Garbage Collection\n";
            std::cout << "  • Independent Memory Spaces\n";
            std::cout << "  • Fault Isolation\n";
            std::cout << "  • No Single Point of Failure\n\n";
            return true;
        } else {
            std::cerr << "\n❌ Linking failed\n";
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "\n❌ COMPILATION ERROR: " << e.what() << std::endl;
        return false;
    }
}

bool Compiler::compile_string(const std::string& source,
                              const std::string& output_file) {
    try {
        // Parse Python string
        ast::PythonASTConverter converter;
        const auto module = converter.parse_string(source);

        // Generate LLVM IR
        codegen::LLVMCodeGen codegen("aithon_module");
        codegen.codegen(module.get());
        codegen.optimize();

        // Emit object file
        std::string object_file = output_file + ".o";
        codegen.emit_object_file(object_file);

        // Link with runtime
        return link_with_runtime(object_file, output_file);

    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
        return false;
    }
}

bool Compiler::compile_to_object(const std::string& input_file,
                                 const std::string& object_file) {
    try {
        ast::PythonASTConverter converter;
        const auto module = converter.parse_file(input_file);

        codegen::LLVMCodeGen codegen("aithon_module");
        codegen.codegen(module.get());
        codegen.optimize();
        codegen.emit_object_file(object_file);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
        return false;
    }
}

bool Compiler::emit_llvm_ir(const std::string& input_file,
                           const std::string& ir_file) {
    try {
        ast::PythonASTConverter converter;
        const auto module = converter.parse_file(input_file);

        codegen::LLVMCodeGen codegen("aithon_module");
        codegen.codegen(module.get());
        codegen.emit_llvm_ir(ir_file);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
        return false;
    }
}

bool Compiler::link_with_runtime(const std::string& object_file,
                                 const std::string& output_file) {
    // Use clang++ to link with runtime library
    // std::string cmd = "clang++ -o " + output_file + " " + object_file + " -L. -laithon_runtime -lpthread";

    // std::string cmd = "clang++ -o " + output_file + " " + object_file;

    // RUNTIME_LIB_DIR is now provided by CMake at compile time
    const std::string lib_dir = RUNTIME_LIB_DIR;

#ifdef __APPLE__
    std::string lib_file = lib_dir + "/libaithon_runtime.dylib";
#else
    std::string lib_file = lib_dir + "/libaithon_runtime.so";
#endif

    // We use -Wl,-rpath to tell the executable where to look for .dylib files at runtime
    const std::string cmd = "clang++ -o \"" + output_file + "\" \"" + object_file + "\" " +
                      "\"" + lib_file + "\" -lpthread " +
                      "\"-Wl,-rpath," + lib_dir + "\""; // Entire flag must be quoted

    std::cout << "Executing: " << cmd << std::endl;
    int result = std::system(cmd.c_str());

    if (result != 0) {
        std::cerr << "Linking failed" << std::endl;
        return false;
    }

    return true;
}

}