#pragma once

#include <string>

namespace pyvm::compiler {

    class Compiler {
    public:
        Compiler();
        ~Compiler();

        // Compile Python file to executable
        bool compile_file(const std::string& input_file,
                         const std::string& output_file);

        // Compile Python string to executable
        bool compile_string(const std::string& source,
                           const std::string& output_file);

        // Compile to object file only
        bool compile_to_object(const std::string& input_file,
                              const std::string& object_file);

        // Emit LLVM IR for debugging
        bool emit_llvm_ir(const std::string& input_file,
                         const std::string& ir_file);

    private:
        bool link_with_runtime(const std::string& object_file,
                              const std::string& output_file);
    };

} // namespace pyvm::compiler