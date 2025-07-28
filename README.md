# MMOC - LLVM-Backed C99 Compiler

A modern C99 compiler built with LLVM and ANTLR4, implementing clean architecture with C++20.

## Features

- Full C99 parsing with ANTLR4-generated parser
- LLVM IR generation for optimized native code
- Command-line driver compatible with GCC-style arguments
- Extensible architecture for C11+ features
- Cross-platform support (Linux, macOS)

## Quick Start

### Prerequisites

**macOS:**
```bash
brew install llvm antlr4 antlr4-cpp-runtime cmake ninja
```

**Ubuntu:**
```bash
sudo apt-get install llvm-17 clang-17 libantlr4-runtime-dev antlr4 cmake ninja-build
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

## Project Structure

```
├── src/
│   ├── driver/          # Main compiler driver
│   ├── parser/          # ANTLR-based parser and AST builder
│   ├── ast/             # AST node definitions
│   ├── sema/            # Semantic analysis
│   ├── codegen/         # LLVM IR generation
│   └── utils/           # Utilities and error handling
├── grammar/             # ANTLR4 C grammar
├── test/                # Unit and integration tests
└── cmake/               # CMake modules
```

## Development Phases

The project is developed in phases following the implementation scope:

- ✅ **Phase 0**: Repository setup and CI
- ✅ **Phase 1**: Toolchain setup
- ✅ **Phase 2**: Grammar and parser foundation
- ✅ **Phase 3**: AST layer
- 🚧 **Phase 4**: Semantic analysis (basic)
- 🚧 **Phase 5**: Code generation MVP
- 📋 **Phase 6+**: Statements, functions, data types...

## Testing

```bash
# Run unit tests
ctest --test-dir build --output-on-failure

# Run integration tests
./build/ccomp test/inputs/hello.c && ./a.out
```

## Contributing

1. Follow C++20 standards with Google style formatting
2. Add unit tests for new features
3. Update documentation for API changes
4. Ensure CI passes on all platforms

## License

[License details to be added]
