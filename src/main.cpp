/**
// ============================================================================
# AIthon Compiler for future AI Programming
# Python code compilation to native machine binary - execute faster, scale better
# Fault-tolerant concurrent system based on Actor-based programming model

## Developed By SSS2FAI (Small Simple Steps towards Future AI) - Gateway for future AI research & learning
## Core Contributor: Furqan Khan
## Email: furqan.cloud.dev@gmail.com
# ============================================================================
**/


#include "compiler/compiler.h"
#include <iostream>

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options] <input.py>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <output>    Specify output file (default: a.out)\n";
    std::cout << "  --emit-llvm    Emit LLVM IR instead of executable\n";
    std::cout << "  --emit-obj     Emit object file only\n";
    std::cout << "  -h, --help     Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << prog_name << " program.py\n";
    std::cout << "  " << prog_name << " -o my_program program.py\n";
    std::cout << "  " << prog_name << " --emit-llvm program.py\n";
}

int main(const int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string input_file;
    std::string output_file = "a.out";
    bool emit_llvm = false;
    bool emit_obj = false;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                return 1;
            }
        } else if (arg == "--emit-llvm") {
            emit_llvm = true;
            if (output_file == "a.out") {
                output_file = "output.ll";
            }
        } else if (arg == "--emit-obj") {
            emit_obj = true;
            if (output_file == "a.out") {
                output_file = "output.o";
            }
        } else if (arg[0] != '-') {
            input_file = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }
    
    if (input_file.empty()) {
        std::cerr << "Error: No input file specified\n";
        print_usage(argv[0]);
        return 1;
    }
    
    std::cout << "AIthon Compiler v0.1.0\n";
    std::cout << "===================\n\n";

    bool success = false;
    
    // if (emit_llvm) {
    //     std::cout << "Compiling " << input_file << " to LLVM IR...\n";
    //     success = aithon::compiler::Compiler::emit_llvm_ir(input_file, output_file);
    // } else if (emit_obj) {
    //     std::cout << "Compiling " << input_file << " to object file...\n";
    //     success = aithon::compiler::Compiler::compile_to_object(input_file, output_file);
    // } else {
    //     std::cout << "Compiling " << input_file << " to executable...\n";
    //     success = aithon::compiler::Compiler::compile_file(input_file, output_file);
    // }

    std::cout << "Compiling " << input_file << " to executable...\n";
    success = aithon::compiler::Compiler::compile_file(input_file, output_file);
    
    if (success) {
        std::cout << "\nCompilation successful!\n";
        std::cout << "Output: " << output_file << "\n";
        return 0;
    } else {
        std::cerr << "\nCompilation failed!\n";
        return 1;
    }
}