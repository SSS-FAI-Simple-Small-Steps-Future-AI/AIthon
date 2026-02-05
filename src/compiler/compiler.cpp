#include "compiler/compiler.h"
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
        // Parse Python file
        std::cout << "Parsing " << input_file << "..." << std::endl;
        ast::PythonASTConverter converter;
        auto module = converter.parse_file(input_file);
        
        // Generate LLVM IR
        std::cout << "Generating LLVM IR..." << std::endl;
        codegen::LLVMCodeGen codegen("pyvm_module");
        codegen.codegen(module.get());
        
        // Optimize
        std::cout << "Optimizing..." << std::endl;
        codegen.optimize();
        
        // Emit object file
        std::string object_file = output_file + ".o";
        std::cout << "Emitting object file..." << std::endl;
        codegen.emit_object_file(object_file);
        
        // Link with runtime
        std::cout << "Linking..." << std::endl;
        return link_with_runtime(object_file, output_file);
        
    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
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