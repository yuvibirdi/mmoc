# MMOC - LLVM-Backed C99 Compiler

A modern C99 compiler built with LLVM and ANTLR4, implementing clean architecture with C++20.

## Features

- Full C99 parsing with ANTLR4-generated parser
- LLVM IR generation for optimized native code
- Command-line driver compatible with GCC-style arguments
- Extensible architecture for C11+ features
- Cross-platform support (Linux, macOS (I don't care about windows))

## Quick Start

### Prerequisites

**macOS:**
```bash
brew install llvm antlr4 antlr4-cpp-runtime cmake ninja
```

### Building

```bash
git clone <repository-url> mmoc
cd mmoc
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target all -j
```

### Usage

```bash
# Compile a C file
./build/ccomp examples/hello.c -o hello

# Generate LLVM IR (debug mode)
./build/ccomp examples/hello.c -d -o hello.ll

# Verbose output
./build/ccomp examples/hello.c -v -o hello
```

## Testing

```bash
# Run compiler test suite
python3 tests/test_runner.py

# Filter tests
python3 tests/test_runner.py -f Pointers

# Run CTest integration smoke test
ctest --test-dir build --output-on-failure
```
