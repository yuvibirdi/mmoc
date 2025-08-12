# MMOC (Making My Own Compiler) - LLVM-Backed C99/C11 Compiler

A modern C99-first compiler (expanding toward C11) built with LLVM and ANTLR4 in C++20.

## Features

Implemented (green tests):
- Core control flow (if/while/for, break/continue)
- Functions & variables
- Arithmetic, comparison, bitwise, logical (short-circuit)
- Ternary operator
- Prefix/Postfix ++/-- (ints)
- Compound assignments
- Pointers (multi-level) & basic arrays
- _Bool type
- Preprocessor (delegated to clang -E) includes & macros
- Basic literals: int, char, string
- Minimal sizeof (primitive/expr returns 4 currently)

Planned next: pointer arithmetic scaling, comma operator, do-while, switch.

## Supported Platforms
- macOS 
- Ubuntu 
- Arch Linux 

## Quick Start
```
### Prerequisites

```bash
./setup.sh # this should dynamcially based on your os, install all the needed packages and configure the project 
```
## Build
```bash
git clone <repository-url> mmoc
cd mmoc
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Usage
```bash
# Compile C source to executable
./build/ccomp file.c -o a.out

# Emit LLVM IR
./build/ccomp file.c -d -o file.ll

# Verbose
./build/ccomp file.c -v -o prog
```

## Testing
```bash
python3 tests/test_runner.py               # all tests
python3 tests/test_runner.py -f Operators  # filter
ctest --test-dir build --output-on-failure # TODO unit tests
```

## CI
GitHub Actions builds on macOS, Ubuntu and Arch. Testing using the Python test harness.

## Development Notes
- Parser: ANTLR4 generated from C.g4 (C11 grammar). ASTBuilder narrows to implemented subset.
- IR: Opaque pointer mode, integer-focused semantics. Future: richer type system & promotions.
- Preprocessing: external clang -E invocation.

## Implementation Status: 
See STATUS.md for the detailed status.