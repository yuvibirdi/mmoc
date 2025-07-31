# MMOC Test Suite Configuration

## Test Organization

This test suite follows LLVM/Clang testing conventions with the following structure:

- `Basic/` - Basic compilation and execution tests
- `Arithmetic/` - Mathematical operations and precedence
- `Variables/` - Variable declarations, assignments, and usage  
- `Functions/` - Function definitions and calls
- `ControlFlow/` - If statements, loops, and control structures

## Test Directives

Tests use standard LLVM-style directives:

- `// RUN: command` - Command to execute for the test
- `// XFAIL: *` - Test expected to fail
- `// UNSUPPORTED: platform` - Test not supported on platform

## Variables

- `%ccomp` - Path to the MMOC compiler
- `%s` - Source file path
- `%t` - Temporary output file
- `%T` - Temporary directory

## Running Tests

```bash
# Run all tests
python3 test_runner.py

# Run specific test category
python3 test_runner.py --filter Basic

# Run with custom compiler path
python3 test_runner.py --compiler ./build/ccomp

# Verbose output
python3 test_runner.py --verbose
```

## Expected Results

Currently working features:
- Basic program compilation
- Arithmetic operations (+, -, *, /, %)
- Variable declarations and assignments
- Function definitions and calls
- If statements with conditions

Known failing features (XFAIL tests):
- While loops (parsing issues with compound statements)
- For loops
- Complex control flow
