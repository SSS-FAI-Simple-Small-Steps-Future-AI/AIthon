#include "../../include/compiler/compiler.h"
#include "../../include/validator/project_validator.h"
#include "../../include/lexer/lexer.h"
#include "../../include/parser/parser.h"
#include "../../include/parser/ast.h"
#include "../../include/utils/error_reporter.h"
#include "../../include/analyzer/semantic_analyzer.h"
#include "../../include/codegen/llvm_codegen.h"
#include <iostream>
#include <fstream>
#include <sstream>

// At top of file:
#ifndef AITHON_RUNTIME_LIB
#define AITHON_RUNTIME_LIB "libaithonruntime.a"
#endif

namespace aithon::compiler {



Compiler::Compiler() = default;

Compiler::~Compiler() = default;

bool Compiler::compile_file(const std::string& input_file,
                            const std::string& output_file) {
    try {
        // ========================================
        // STEP 1: Validate Project Structure
        // ========================================
        std::cout << "╔════════════════════════════════════════════╗\n";
        std::cout << "║   AIthon Compiler - Custom Frontend Mode    ║\n";
        std::cout << "╚════════════════════════════════════════════╝\n\n";

        auto validation_result = validator::ProjectValidator::run_all_validations(input_file);

        if (!validation_result.is_valid) {
            std::cerr << "\n❌ COMPILATION STOPPED: Validation failed\n";
            std::cerr << validation_result.error_message << std::endl;
            return false;
        }

        std::string main_file = validation_result.main_file_path;

        // ========================================
        // STEP 2: Read Source File
        // ========================================
        std::cout << "=== Compilation Pipeline ===" << std::endl;
        std::cout << "[1/7] Reading source file..." << std::endl;

        std::ifstream file(main_file);
        if (!file) {
            std::cerr << "❌ Could not open file: " << main_file << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source_code = buffer.str();
        file.close();

        std::cout << "✓ Source file read (" << source_code.size() << " bytes)\n\n";

       // Create error reporter
    utils::ErrorReporter error_reporter(source_code, "filename");

    // ========================================================================
    // STEP 1: Lexical Analysis
    // ========================================================================
    std::cout << "=== [1/5] Lexical Analysis ===\n";
    lexer::Lexer lexer(source_code, error_reporter);
    std::vector<lexer::Token> tokens = lexer.tokenize();

    if (error_reporter.has_errors()) {
        std::cerr << "❌ Lexing failed\n";
        return 1;
    }

    std::cout << "✓ Tokenized " << tokens.size() << " tokens\n\n";

    // ========================================================================
    // STEP 2: Parsing
    // ========================================================================
    std::cout << "=== [2/5] Parsing ===\n";
    parser::Parser parser(tokens, error_reporter);
    auto ast = parser.parse();

    if (error_reporter.has_errors() || !ast) {
        std::cerr << "❌ Parsing failed\n";
        return false;
    }

    std::cout << "✓ AST generated\n\n";

    // ========================================================================
    // STEP 3: Semantic Analysis
    // ========================================================================
    std::cout << "=== [3/5] Semantic Analysis ===\n";
    analyzer::SemanticAnalyzer semantic_analyzer(error_reporter);
    bool semantic_ok = semantic_analyzer.analyze(ast.get());

    if (!semantic_ok || error_reporter.has_errors()) {
        std::cerr << "❌ Semantic analysis failed\n";
        return false;
    }

    std::cout << "✓ Type checking passed\n";
    std::cout << "✓ All variables defined\n";
    std::cout << "✓ Function returns validated\n\n";

    // ========================================================================
    // STEP 4: LLVM IR Generation
    // ========================================================================
    std::cout << "=== [4/5] LLVM IR Generation ===\n";
    codegen::LLVMCodeGen codegen(error_reporter);
    bool success = codegen.generate(ast.get(), "aithon_module");
    if (!success) {
        std::cerr << "❌ Code generation failed\n";
        return false;
    }

    std::cout << "✓ LLVM IR generated\n";

    // Write IR to file
    std::string ir_file = output_file + ".ll";
    if (!codegen.write_ir_to_file(ir_file)) {
        std::cerr << "❌ Failed to write IR file\n";
        return false;
    }
    std::cout << "✓ IR written to: " << ir_file << "\n\n";

    // ========================================================================
    // STEP 5: Object Code Generation
    // ========================================================================
    std::cout << "=== [5/5] Object Code Generation ===\n";

    std::string obj_file = output_file + ".o";
    if (!codegen.write_object_to_file(obj_file)) {
        std::cerr << "❌ Failed to generate object file\n";
        return false;
    }
    std::cout << "✓ Object file: " << obj_file << "\n";

    std::string runtime_cmd = "clang++ -c runtime.cpp -o runtime.o";
    std::system(runtime_cmd.c_str());

    std::string exe_file = output_file;

    // Link to create executable
    bool link_success = link_executable(obj_file, exe_file);
    if (!link_success) {
        std::cerr << "❌ Linking failed\n";
        return false;
    }

    // std::string exe_file = output_file;
    // std::string link_cmd = "clang++ -o " + exe_file + " " + obj_file + " runtime.o";
    //
    // std::cout << "✓ Linking...\n";
    // std::cout << "  Command: " << link_cmd << "\n";
    //
    // int link_result = std::system(link_cmd.c_str());
    // if (link_result != 0) {
    //     std::cerr << "❌ Linking failed\n";
    //     std::cerr << "  Make sure runtime.cpp is compiled: clang++ -c runtime.cpp -o runtime.o\n";
    //     return 1;
    // }

    std::cout << "✓ Executable: " << exe_file << "\n\n";

    std::cout << "✅ Compilation successful!\n\n";
    std::cout << "Run your program:\n";
    std::cout << "  ./" << exe_file << "\n";

        // // ========================================
        // // STEP 10: Link with Runtime
        // // ========================================
        // std::cout << "Linking with PyVM runtime..." << std::endl;
        //
        // bool linked = link_with_runtime(object_file, output_file);
        //
        // if (linked) {
        //     std::cout << "\n╔════════════════════════════════════════════╗\n";
        //     std::cout << "║        ✓ COMPILATION SUCCESSFUL!           ║\n";
        //     std::cout << "╚════════════════════════════════════════════╝\n\n";
        //     std::cout << "Executable: " << output_file << "\n";
        //     std::cout << "\nLanguage Features:\n";
        //     std::cout << "  • Custom Lexer & Parser (Zero Python Dependency)\n";
        //     std::cout << "  • Block-Scoped Variables\n";
        //     std::cout << "  • Stack-Allocated Scalars\n";
        //     std::cout << "  • Immutable Strings\n";
        //     std::cout << "  • Classes (No Inheritance)\n";
        //     std::cout << "\nRuntime Features:\n";
        //     std::cout << "  • Green Threads (M:N threading)\n";
        //     std::cout << "  • Actor-based Concurrency\n";
        //     std::cout << "  • Per-Actor Garbage Collection\n";
        //     std::cout << "  • Zero-Copy Arena Allocators\n";
        //     std::cout << "  • Independent Memory Spaces\n";
        //     std::cout << "  • Fault Isolation\n\n";
        //
        //     // Clean up object file
        //     std::remove(object_file.c_str());
        //
        //     return true;
        // } else {
        //     std::cerr << "\n❌ Linking failed\n";
        //     return false;
        // }

    } catch (const std::exception& e) {
        std::cerr << "\n❌ COMPILATION ERROR: " << e.what() << std::endl;
        return false;
    }
    return true;
}


// In your linking function:
bool Compiler::link_executable(const std::string& obj_file,
                               const std::string& output_exe) {

    std::string runtime_lib = AITHON_RUNTIME_LIB;

    if (!std::filesystem::exists(runtime_lib)) {
        std::cerr << "ERROR: Runtime library not found!\n";
        std::cerr << "Run: cmake .. && make\n";
        return false;
    }

    // std::string link_cmd = "clang++ -o " + output_exe + " " +
    //                       obj_file + " " + runtime_lib;

    // ✅ FIXED - quotes protect spaces:
    std::string link_cmd = "clang++ -o \"" + output_exe + "\" \"" +
                          obj_file + "\" \"" + runtime_lib + "\"";

    return std::system(link_cmd.c_str()) == 0;
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