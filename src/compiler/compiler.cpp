#include "compiler/compiler.h"
#include "validator/project_validator.h"
#include "ast/python_ast_converter.h"
#include "codegen/llvm_codegen.h"
#include <iostream>
#include <cstdlib>

namespace pyvm::compiler {

Compiler::Compiler() {}

Compiler::~Compiler() {}

bool Compiler::compile_file(const std::string& input_file,
                            const std::string& output_file) {
    try {
        // ========================================
        // STEP 1: Validate Project Structure
        // ========================================
        std::cout << "╔════════════════════════════════════════════╗\n";
        std::cout << "║   PyVM Compiler - Strict Validation Mode   ║\n";
        std::cout << "╚════════════════════════════════════════════╝\n\n";

        auto validation_result = validator::ProjectValidator::run_all_validations(input_file);

        if (!validation_result.is_valid) {
            std::cerr << "\n❌ COMPILATION STOPPED: Validation failed\n";
            std::cerr << validation_result.error_message << std::endl;
            return false;
        }

        std::string main_file = validation_result.main_file_path;

        // ========================================
        // STEP 2: Parse Python 3.12 Code to AST
        // ========================================
        std::cout << "=== Compilation Pipeline ===" << std::endl;
        std::cout << "[1/4] Parsing Python code to AST..." << std::endl;
        ast::PythonASTConverter converter;
        auto module = converter.parse_file(main_file);
        std::cout << "✓ AST generated successfully\n\n";

        // ========================================
        // STEP 3: Generate LLVM IR from AST
        // ========================================
        std::cout << "[2/4] Generating LLVM IR..." << std::endl;
        codegen::LLVMCodeGen codegen("pyvm_module");
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
        std::string object_file = output_file + ".o";
        codegen.emit_object_file(object_file);
        std::cout << "✓ Object file created: " << object_file << "\n\n";

        // Link with runtime
        std::cout << "Linking with PyVM runtime..." << std::endl;
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
        auto module = converter.parse_string(source);
        
        // Generate LLVM IR
        codegen::LLVMCodeGen codegen("pyvm_module");
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
        auto module = converter.parse_file(input_file);
        
        codegen::LLVMCodeGen codegen("pyvm_module");
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
        auto module = converter.parse_file(input_file);
        
        codegen::LLVMCodeGen codegen("pyvm_module");
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
    std::string cmd = "clang++ -o " + output_file + " " + object_file + 
                     " -L. -lpyvm_runtime -lpthread";
    
    std::cout << "Executing: " << cmd << std::endl;
    int result = std::system(cmd.c_str());
    
    if (result != 0) {
        std::cerr << "Linking failed" << std::endl;
        return false;
    }
    
    return true;
}

} // namespace pyvm::compiler