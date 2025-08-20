# MMOC (Making My Own Compiler) - LLVM-Backed C99/C11 Compiler

A modern C99-first compiler (expanding toward C11) built with LLVM and ANTLR4 in C++20.

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
./build/mmoc file.c -o a.out

# Emit LLVM IR
./build/mmoc file.c -d -o file.ll

# Verbose
./build/mmoc file.c -v -o prog
```

## Testing
```bash
ython3 tests/test_runner.py               # all tests
python3 tests/test_runner.py -f Operators  # filter
ctest --test-dir build --output-on-failure # TODO: UNIT TESTS NOT YET IMPLEMENTED
```

## CI
GitHub Actions builds on macOS, Ubuntu and Arch. Testing using the Python test harness.

## Compatiblity Status: 
See STATUS.md