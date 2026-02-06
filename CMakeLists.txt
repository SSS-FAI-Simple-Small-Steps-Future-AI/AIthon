cmake_minimum_required(VERSION 4.1)
project(AIthon)

set(CMAKE_CXX_STANDARD 20)

# add_executable(AIthon main.cpp)

# 1. Tell CMake where to look for LLVM (Homebrew path)
# Apple Silicon: /opt/homebrew/opt/llvm/lib/cmake/llvm
# Intel Mac: /usr/local/opt/llvm/lib/cmake/llvm

# Dynamically find the Homebrew LLVM path
execute_process(COMMAND brew --prefix llvm OUTPUT_VARIABLE LLVM_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
list(APPEND CMAKE_PREFIX_PATH "${LLVM_PREFIX}/lib/cmake/llvm")

# 2. Find LLVM components
find_package(LLVM REQUIRED CONFIG)

# 3. Add LLVM definitions and include directories
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# 4. Define your executable
add_executable(AIthon main.cpp)

# 5. Link against LLVM libraries (e.g., Core, IRReader, Support)
llvm_map_components_to_libnames(llvm_libs core support native codegen)
target_link_libraries(AIthon ${llvm_libs})
