# MMOC - LLVM-Backed C99/C11 Progressing Compiler

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
- macOS (Apple Silicon / Intel)
- Ubuntu (CI: ubuntu-latest)
- Arch Linux (CI container job)

## Prerequisites

### macOS (Homebrew)
```bash
brew install llvm antlr4 antlr4-cpp-runtime cmake ninja
```
Add to PATH (optional):
```bash
echo "export PATH=\"$(brew --prefix llvm)/bin:$PATH\"" >> ~/.bashrc
```

### Ubuntu (22.04+)
```bash
sudo apt update
sudo apt install -y llvm-17 llvm-17-dev clang-17 lld-17 \
  libantlr4-runtime-dev antlr4 pkg-config ninja-build python3
sudo ln -sf /usr/bin/llvm-config-17 /usr/bin/llvm-config
sudo ln -sf /usr/bin/clang-17 /usr/bin/clang
sudo ln -sf /usr/bin/clang++-17 /usr/bin/clang++
```

### Arch Linux
```bash
sudo pacman -S --needed base-devel llvm clang lld cmake ninja antlr4-runtime python
```
If antlr4 runtime headers not found, install from AUR (antlr4-cpp-runtime) or build locally.

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
ctest --test-dir build --output-on-failure
```

## CI
GitHub Actions builds on macOS, Ubuntu (Debug & Release) and Arch (Release). Runs Python test harness and CTest.

## Development Notes
- Parser: ANTLR4 generated from C.g4 (C11 grammar). ASTBuilder narrows to implemented subset.
- IR: Opaque pointer mode, integer-focused semantics. Future: richer type system & promotions.
- Preprocessing: external clang -E invocation.

## Roadmap Snapshot
See STATUS.md for detailed ordering.

## License
BSD-based upstream grammar components; project code MIT (add LICENSE file TBD).
