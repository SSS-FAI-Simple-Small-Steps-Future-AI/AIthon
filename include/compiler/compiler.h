#pragma once

#include <string>

namespace aithon::compiler {

    class Compiler {
    public:
        Compiler();
        ~Compiler();

        // Compile Python file to executable
        static bool compile_file(const std::string& input_file,
                         const std::string& output_file);



        // // Compile Python string to executable
        // static bool compile_string(const std::string& source,
        //                    const std::string& output_file);
        //
        // // Compile to object file only
        // static bool compile_to_object(const std::string& input_file,
        //                       const std::string& object_file);
        //
        // // Emit LLVM IR for debugging
        // static bool emit_llvm_ir(const std::string& input_file,
        //                  const std::string& ir_file);

    private:
        static bool link_with_runtime(const std::string& object_file,
                              const std::string& output_file);

        static bool link_executable(const std::string& obj_file,
                               const std::string& output_exe);
    };

}