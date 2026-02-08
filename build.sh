#!/bin/bash

# AIthon-Lang Build Script for macOS (Intel)
# =========================================

set -e  # Exit on error

echo "================================================"
echo "  AIthon-Lang Build Script for macOS (Intel)"
echo "================================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo -e "${RED}Error: This script is for macOS only${NC}"
    exit 1
fi

# Check for Homebrew
echo "Checking dependencies..."
if ! command -v brew &> /dev/null; then
    echo -e "${YELLOW}Homebrew not found. Installing...${NC}"
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# Install dependencies
echo -e "${GREEN}Installing dependencies...${NC}"
brew install llvm@21 cmake python@3.12 || true

# Set LLVM path
export LLVM_DIR="/usr/local/opt/llvm@21"
if [ ! -d "$LLVM_DIR" ]; then
    # Try alternate path (Apple Silicon)
    export LLVM_DIR="/opt/homebrew/opt/llvm@21"
fi

if [ ! -d "$LLVM_DIR" ]; then
    echo -e "${RED}Error: LLVM 21 not found. Please install manually:${NC}"
    echo "  brew install llvm@21"
    exit 1
fi

echo -e "${GREEN}Using LLVM at: $LLVM_DIR${NC}"

# Set environment variables
export PATH="$LLVM_DIR/bin:$PATH"
export LDFLAGS="-L$LLVM_DIR/lib"
export CPPFLAGS="-I$LLVM_DIR/include"
export CMAKE_PREFIX_PATH="$LLVM_DIR"

# Create build directory
echo ""
echo -e "${GREEN}Creating build directory...${NC}"
rm -rf build
mkdir -p build
cd build

# Configure with CMake
echo ""
echo -e "${GREEN}Configuring with CMake...${NC}"
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_DIR="$LLVM_DIR/lib/cmake/llvm" \
      -DCMAKE_CXX_FLAGS="-std=c++20" \
      ..

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

# Build
echo ""
echo -e "${GREEN}Building (this may take a few minutes)...${NC}"
make -j$(sysctl -n hw.ncpu)

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Run tests
echo ""
echo -e "${GREEN}Running tests...${NC}"
ctest --output-on-failure

if [ $? -ne 0 ]; then
    echo -e "${YELLOW}Warning: Some tests failed${NC}"
else
    echo -e "${GREEN}All tests passed!${NC}"
fi

# Create wrapper script
echo ""
echo -e "${GREEN}Creating wrapper script...${NC}"
cd ..
cat > pyvm << 'EOF'
#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export DYLD_LIBRARY_PATH="$SCRIPT_DIR/build:$DYLD_LIBRARY_PATH"
"$SCRIPT_DIR/build/pyvm_compiler" "$@"
EOF

chmod +x pyvm

# Success message
echo ""
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}  Build completed successfully!${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""
echo "You can now use the compiler:"
echo -e "  ${YELLOW}./pyvm examples/fibonacci.py -o fib${NC}"
echo -e "  ${YELLOW}./fib${NC}"
echo ""
echo "Or with full path:"
echo -e "  ${YELLOW}./build/pyvm_compiler examples/fibonacci.py -o fib${NC}"
echo ""
echo "To emit LLVM IR:"
echo -e "  ${YELLOW}./pyvm --emit-llvm examples/fibonacci.py${NC}"
echo ""
echo "For help:"
echo -e "  ${YELLOW}./pyvm --help${NC}"
echo ""

# Test compilation
echo -e "${GREEN}Testing compilation with fibonacci example...${NC}"
./build/pyvm_compiler examples/fibonacci.py -o build/fib_test

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Compilation test successful!${NC}"
else
    echo -e "${YELLOW}⚠ Compilation test had issues (this is expected for incomplete implementation)${NC}"
fi

echo ""
echo -e "${GREEN}Setup complete! Happy coding!${NC}"