# AIthon Compiler Quick Start Guide

## Installation (macOS Intel)

### Automated Installation

```bash
cd pyvm-lang
./build.sh
```

This script will:
1. Install dependencies (LLVM 21, CMake, Python 3.11)
2. Configure and build the project
3. Run tests
4. Create wrapper scripts

### Manual Installation

```bash
# Install dependencies
brew install llvm@21 cmake python@3.11

# Set environment
export PATH="/usr/local/opt/llvm@21/bin:$PATH"
export LLVM_DIR="/usr/local/opt/llvm@21"

# Build
mkdir build && cd build
cmake -DLLVM_DIR=/usr/local/opt/llvm@21/lib/cmake/llvm ..
make -j4
```

## Your First Program

Create `hello.py`:

```python
def main():
    x = 10
    y = 20
    result = x + y
    print(result)

main()
```

Compile and run:

```bash
./pyvm hello.py -o hello
./hello
```

Output:
```
30
```

## Examples

### 1. Fibonacci (Recursive)

```python
# examples/fibonacci.py
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

def main():
    result = fibonacci(10)
    print(result)

main()
```

Compile:
```bash
./pyvm examples/fibonacci.py -o fib
./fib
# Output: 55
```

### 2. Simple Loop

```python
# loop.py
def count_to_ten():
    i = 0
    while i < 10:
        print(i)
        i = i + 1

count_to_ten()
```

### 3. Conditional Logic

```python
# conditional.py
def check_number(n):
    if n > 0:
        print(1)  # positive
    if n < 0:
        print(2)  # negative

check_number(5)
check_number(3)
```

### 4. Async Function (Actor-based)

```python
# async_example.py
async def worker(x):
    result = x * 2
    return result

async def main():
    result = await worker(21)
    print(result)

main()
```

## Command Line Options

```bash
# Compile to executable
./pyvm program.py -o output

# Emit LLVM IR (for debugging)
./pyvm --emit-llvm program.py -o program.ll

# Emit object file only
./pyvm --emit-obj program.py -o program.o

# Help
./pyvm --help
```

## Viewing LLVM IR

```bash
# Generate IR
./pyvm --emit-llvm examples/fibonacci.py -o fib.ll

# View the IR
cat fib.ll

# Or use LLVM tools
llvm-dis fib.ll
```

## Debugging

### Check Generated Code

```bash
# See LLVM IR
./pyvm --emit-llvm program.py -o program.ll
cat program.ll

# See assembly
./pyvm --emit-obj program.py -o program.o
objdump -d program.o
```

### Runtime Debugging

```bash
# Use lldb
lldb ./program
(lldb) run
(lldb) bt  # backtrace on crash
```

## Troubleshooting

### LLVM Not Found

```bash
# Verify LLVM installation
brew list llvm@21
brew --prefix llvm@21

# Set LLVM_DIR manually
export LLVM_DIR="$(brew --prefix llvm@21)"
cmake -DLLVM_DIR="$LLVM_DIR/lib/cmake/llvm" ..
```

### Python Headers Not Found

```bash
# Install Python development headers
brew install python@3.11

# Verify Python3 is found
python3 --version
python3-config --includes
```

### Compilation Errors

```bash
# Clean build
rm -rf build
mkdir build && cd build
cmake .. && make clean && make
```

### Link Errors

```bash
# Ensure runtime library is built
make pyvm_runtime

# Check library path
export DYLD_LIBRARY_PATH="$PWD:$DYLD_LIBRARY_PATH"
```

## Testing

### Run All Tests

```bash
cd build
make test
```

### Run Specific Tests

```bash
./build/tests/test_scheduler
./build/tests/test_actors
```

### Test Actor Runtime

```cpp
// test_my_actors.cpp
#include "runtime/scheduler.h"

void my_behavior(ActorProcess* self, void* args) {
    std::cout << "Hello from actor " << self->pid() << std::endl;
}

int main() {
    Scheduler sched(4);
    sched.spawn(my_behavior);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    sched.shutdown();
}
```

## Performance Tips

### 1. Increase Worker Threads

For CPU-intensive workloads:

```cpp
// Modify in runtime
Scheduler sched(8);  // 8 workers instead of default
```

### 2. Larger Actor Heaps

For memory-intensive actors:

```cpp
// 10MB instead of default 1MB
sched.spawn(behavior, args, 10 * 1024 * 1024);
```

### 3. Enable Optimizations

```bash
# Build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## What Works Now

✅ Basic integer arithmetic
✅ Function definitions and calls
✅ If statements
✅ While loops
✅ Print statements
✅ Async function declarations
✅ Actor spawning
✅ Message passing (basic)

## Known Limitations

❌ No strings (partial support)
❌ No lists/dictionaries
❌ No classes
❌ No imports
❌ No exceptions
❌ No generators
❌ Limited Python standard library

## Next Steps

1. Read the full README.md for architecture details
2. Explore examples/ directory
3. Modify existing examples
4. Write your own simple programs
5. Check the runtime implementation in runtime/

## Getting Help

- Check README.md for detailed documentation
- Look at examples/ for working code
- Read source code comments
- Open issues for bugs or questions

## Contributing

Want to add features? Great!

1. Pick a Python feature to implement
2. Update AST definitions
3. Add parser support
4. Implement codegen
5. Write tests
6. Submit PR




This way:
- **C `main()`** - Entry point, returns `int` ✅
- **`python_main()`** - Your Python code, returns `int64_t` ✅
- No naming conflict ✅

The flow is:
```
OS → C main() → python_main() (LLVM generated) → return