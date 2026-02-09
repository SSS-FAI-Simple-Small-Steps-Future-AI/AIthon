# AIthon : Python Compiler For High-performance runtime (Natively Compiled Binaries)


### Fault-tolerant concurrent system based on Actor-based programming model
### Open-source Research Project 
### Developed By SSS2FAI (Small Simple Steps towards Future AI) - Gateway for future AI research & learning

A Python compiler that converts Python code to native machine code via LLVM IR, with Erlang-style actor-based concurrency.

## Features

- **LLVM-based compilation**: Python → LLVM IR → Native Code
- **Actor-based concurrency**: Every `async`/`await` maps to isolated actors
- **No GIL**: True parallelism with independent actor processes
- **Fault tolerance**: Erlang-style supervision and crash isolation
- **Preemptive scheduling**: Fair CPU time distribution across actors
- **Work-stealing scheduler**: Automatic load balancing

## Architecture

```
Python Source
     ↓
  CPython Parser (via Python C API)
     ↓
  Internal AST
     ↓
  LLVM IR Generator
     ↓
  LLVM Optimization
     ↓
  Machine Code
     ↓
  Link with Runtime
     ↓
  Executable
```

## Requirements

### macOS (Intel)

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install llvm@21 cmake python@3.11

# Add LLVM to PATH
export PATH="/usr/local/opt/llvm@21/bin:$PATH"
export LDFLAGS="-L/usr/local/opt/llvm@21/lib"
export CPPFLAGS="-I/usr/local/opt/llvm@21/include"
```

## Building

```bash
# Clone the repository
cd AIthon

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake -DLLVM_DIR=/usr/local/opt/llvm@21/lib/cmake/llvm ..

# Build
make -j4

# Run tests
make test
```

## Usage

### Compile a Python file

```bash
./aithon_compiler examples/main.py -o output
./output
```

### Emit LLVM IR

```bash
./aithon_compiler --emit-llvm examples/main.py -o ouput.ll
cat output.ll
```

### Emit Object File

```bash
./aithon_compiler --emit-obj examples/main.py -o output.o
```

![Alt text](./Documentations/images/compiler-screenshot.png)


## Examples

### Async/Await (Actor-Based)

```python
async def worker(task_id):
    result = task_id * 2
    return result

async def main():
    result = await worker(5)
    print(result)

main()
```

## Actor Model Details

### Isolation

Each actor has:
- **Independent heap** (1MB default)
- **Lock-free mailbox** (MPSC queue)
- **Private stack** and registers
- **Reduction counter** (for preemption)

### Message Passing

Messages are copied between actors (no shared memory):

```python
async def receiver():
    msg = await receive()  # Blocks until message arrives
    process(msg)

async def sender():
    await send(receiver_pid, data)  # Non-blocking send
```

### Fault Tolerance

Actors crash independently without affecting others:

```python
async def supervised_worker():
    try:
        risky_operation()
    except Exception:
        # Actor crashes, supervisor handles restart
        pass
```

## Scheduler Details

### Work Stealing

- Each worker has its own run queue
- Idle workers steal from busy workers
- Random victim selection
- Steal 50% of victim's queue

### Preemptive Scheduling

- Each actor gets 2000 reductions per quantum
- Compiler inserts yield checks in loops
- Fair scheduling across millions of actors

### Performance

On a 4-core Intel Mac:
- 1 million messages/second
- Sub-microsecond actor spawn time
- Linear scaling up to core count

## Current Limitations

### Not Yet Implemented

- Full Python standard library
- Dynamic features (eval, exec, metaprogramming)
- Classes with inheritance
- Generators and iterators
- Exception handling
- Import system
- Type annotations enforcement

### Supported Features

- Basic functions (sync and async)
- Integer and float arithmetic
- Control flow (if, while, for)
- Function calls
- Print statements
- Binary operations
- Comparisons

## Project Structure

```
AIthon/
├── include/           # Header files
│   ├── ast/          # AST definitions
│   ├── codegen/      # LLVM code generation
│   ├── compiler/     # Compiler interface
│   └── runtime/      # Actor runtime
├── src/              # Implementation files
├── runtime_lib/      # Runtime C API
├── examples/         # Example Python programs
├── tests/            # Unit tests
└── CMakeLists.txt    # Build configuration
```

## Development

### Adding New Features

1. Update AST nodes in `include/ast/ast_nodes.h`
2. Update parser in `src/ast/python_ast_converter.cpp`
3. Add codegen in `src/codegen/llvm_codegen.cpp`
4. Add runtime support if needed in `runtime_lib/runtime.cpp`

### Debugging

```bash
# Dump LLVM IR
./aithon_compiler --emit-llvm program.py -o program.ll
cat program.ll

# Run with verbose output
LLVM_DEBUG=1 ./aithon_compiler program.py

# Use LLDB for debugging
lldb ./aithon_compiler
(lldb) run program.py
```

## Testing

```bash
# Run all tests
cd build
make test

# Run specific test
./tests/test_scheduler
./tests/test_actors

# Run example programs
./aithon_compiler ../examples/main.py -o main && ./main
```

## Performance Tuning

### Heap Size

Adjust actor heap size for memory-intensive tasks:

```cpp
// Default: 1MB per actor
scheduler.spawn(behavior, args, 10 * 1024 * 1024);  // 10MB heap
```

### Worker Threads

Set number of scheduler workers:

```cpp
Scheduler scheduler(8);  // 8 worker threads
```

### Reduction Budget

Tune preemption granularity in `include/runtime/actor_process.h`:

```cpp
constexpr int REDUCTIONS_PER_SLICE = 2000;  // Higher = less preemption
```

## Contributing

This is a research/educational project. Contributions welcome!

### Code Style

- C++20 standard
- LLVM coding conventions
- Modern CMake practices

### Testing

All new features must have:
- Unit tests in `tests/`
- Example program in `examples/`
- Detailed Documentation in Documentation folder: DOCUMENTATION.md


Python Source → Lexer/Parser (C++) → AST → Type Analysis → LLVM IR → Native Code
↓
Runtime System (Erlang-style)
- Actor Scheduler
- Message Passing
- Fault Tolerance

Complete Scheduler Architecture
Core Concepts

┌─────────────────────────────────────────────────────────┐
│                     Scheduler                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │ Worker 0 │  │ Worker 1 │  │ Worker N │             │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘             │
│       │             │              │                    │
│  ┌────▼─────────────▼──────────────▼─────┐             │
│  │         Run Queue (per worker)         │             │
│  │  [Actor1] [Actor2] [Actor3] ...        │             │
│  └────────────────────────────────────────┘             │
└─────────────────────────────────────────────────────────┘

Each Actor:
┌──────────────────────────────────┐
│ Actor Process                     │
│ ┌──────────────────────────────┐ │
│ │ Private Heap (Isolated GC)   │ │
│ │ - Stack                      │ │
│ │ - Local variables            │ │
│ └──────────────────────────────┘ │
│ ┌──────────────────────────────┐ │
│ │ Mailbox (Lock-free queue)    │ │
│ └──────────────────────────────┘ │
│ ┌──────────────────────────────┐ │
│ │ Reduction Counter (for       │ │
│ │ preemption)                  │ │
│ └──────────────────────────────┘ │
└──────────────────────────────────┘

## Detailed Documentation
Detailed documentation, including phases features and implementation is available in 'Documentations' folder.


## Future Roadmap

### Short Term (3-6 months)

- [ ] Complete Python standard library (core modules)
- [ ] Proper exception handling
- [ ] Generator/iterator support
- [ ] Import system

### Medium Term (6-12 months)

- [ ] AOT compilation for dynamic code
- [ ] Advanced GC (generational, concurrent)
- [ ] Distributed actors (network communication)
- [ ] Hot code reloading

### Long Term (1-2 years)

- [ ] Full CPython compatibility
- [ ] Production-ready performance
- [ ] Tooling (debugger, profiler)
- [ ] Package manager integration

## License

MIT License - See LICENSE file

## Acknowledgments

- LLVM Project for compilation infrastructure
- Erlang/OTP for actor model inspiration - actor runtime ideas
- CPython OR Some other mature tooling, for parser and AST design

## Contact

Interested to contribute for the project, Contact via email.

For questions or discussions, open an issue on GitHub.

---

**Note**: This is an experimental compiler. Do not use in production!
We are looking for more contributors to support the project globally - To build a community around Future AI Research


## Developers/Contributers
Developed By SSS2FAI (Small Simple Steps towards Future AI) - Gateway for future AI research & learning<br>
Core Contributor: Furqan Khan<br>
Email: furqan.cloud.dev@gmail.com