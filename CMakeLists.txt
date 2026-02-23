cmake_minimum_required(VERSION 4.1)
project(AIthon VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Warning flags
#if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#    add_compile_options(-Wall -Wextra -Wpedantic)
#endif()

# 1. Tell CMake where to look for LLVM (Homebrew path)
# Apple Silicon: /opt/homebrew/opt/llvm/lib/cmake/llvm
# Intel Mac: /usr/local/opt/llvm/lib/cmake/llvm

# Dynamically find the Homebrew LLVM path
execute_process(COMMAND brew --prefix llvm OUTPUT_VARIABLE LLVM_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
list(APPEND CMAKE_PREFIX_PATH "${LLVM_PREFIX}/lib/cmake/llvm")

# Find LLVM Components
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)


# BEFORE add_executable(aithon_compiler ...)
add_library(aithon_runtime STATIC
        src/runtime/runtime.cpp
)

# ✅ ADD THESE LINES
target_compile_definitions(aithon_runtime PRIVATE
        __STDC_CONSTANT_MACROS
        __STDC_FORMAT_MACROS
        __STDC_LIMIT_MACROS
)


set_target_properties(aithon_runtime PROPERTIES
        OUTPUT_NAME "aithonruntime"
)

# Your existing add_executable stays the same

# ============================================================================
# Compiler Executable
# ============================================================================


# Compiler sources (NO Python dependency!)
set(COMPILER_SOURCES
        src/main.cpp
        src/compiler/compiler.cpp
        src/lexer/lexer.cpp
        src/lexer/token.cpp
        src/parser/parser.cpp
        src/parser/ast.cpp
        src/analyzer/semantic_analyzer.cpp
        src/codegen/llvm_codegen.cpp
        src/validator/project_validator.cpp
        src/utils/error_reporter.cpp
#        src/codegen/async_transformer.cpp
#        src/codegen/async_to_actor.cpp
)

# Runtime sources
#set(RUNTIME_SOURCES
#        src/runtime/actor_process.cpp
#        src/runtime/scheduler.cpp
#        src/runtime/heap.cpp
#        src/runtime/pyobject.cpp
#        src/runtime/exceptions.cpp
#        src/runtime/green_threads.cpp
#        src/runtime/actor_gc.cpp
#)

# LLVM components we need
llvm_map_components_to_libnames(llvm_libs
        core
        irreader
        support
        native
        orcjit
        passes
        target
        transformutils
        analysis
        ipo
        instcombine
        scalaropts
        vectorize
)

# Main compiler executable
#add_executable(aithon_compiler ${COMPILER_SOURCES} ${RUNTIME_SOURCES})
add_executable(aithon_compiler ${COMPILER_SOURCES})


# Add this line to your CMakeLists.txt
target_compile_definitions(aithon_compiler PRIVATE
        RUNTIME_LIB_DIR="${CMAKE_BINARY_DIR}"
)

target_link_libraries(aithon_compiler
        ${llvm_libs}
        aithon_runtime
#        pthread
)

target_compile_definitions(aithon_compiler PRIVATE
        AITHON_RUNTIME_LIB="$<TARGET_FILE:aithon_runtime>"
)

add_dependencies(aithon_compiler aithon_runtime)




## Runtime shared library
#add_library(aithon_runtime SHARED
#        runtime_lib/runtime.cpp
#)

# Force the library to be output directly in the build directory
set_target_properties(aithon_runtime PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

#target_link_libraries(aithon_runtime pthread)
#
#add_dependencies(aithon_compiler aithon_runtime)

# Tests
#enable_testing()
#add_subdirectory(tests)

# Install targets
install(TARGETS aithon_compiler DESTINATION bin)
install(TARGETS aithon_compiler DESTINATION lib)

# ============================================================================
# Testing
# ============================================================================

#enable_testing()
#
## Test 1: Lexer test
#add_executable(test_lexer tests/test_lexer.cpp src/parser/lexer.cpp)
#target_include_directories(test_lexer PRIVATE ${CMAKE_SOURCE_DIR}/include)
#add_test(NAME LexerTest COMMAND test_lexer)
#
## Test 2: Parser test
#add_executable(test_parser
#        tests/test_parser.cpp
#        src/parser/lexer.cpp
#        src/parser/parser.cpp
#)
#target_include_directories(test_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)
#add_test(NAME ParserTest COMMAND test_parser)
#
## Test 3: Semantic analyzer test
#add_executable(test_semantic
#        tests/test_semantic.cpp
#        src/parser/lexer.cpp
#        src/parser/parser.cpp
#        src/semantic/analyzer.cpp
#)
#target_include_directories(test_semantic PRIVATE ${CMAKE_SOURCE_DIR}/include)
#add_test(NAME SemanticTest COMMAND test_semantic)

# ============================================================================
# Build Configuration
# ============================================================================





# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "AIthon Compiler Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  LLVM Version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Components:")
message(STATUS "  Compiler: aithon_compiler")
message(STATUS "  Runtime: libaithon_runtime")
message(STATUS "  Tests: test_lexer, test_parser, test_semantic")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  • Custom Lexer & Parser (Zero Python Dependency)")
message(STATUS "  • Block-Scoped Variables")
message(STATUS "  • Semantic Analysis")
message(STATUS "  • LLVM IR Generation")
message(STATUS "  • Native Code Compilation")
message(STATUS "")