# Changelog

## [0.1.0] - 2025-01-08

### Initial Release

#### Features
- C99/C11 compiler with LLVM backend
- Core control flow (if/while/for, break/continue)
- Functions and variables
- Arithmetic, comparison, bitwise, logical operators
- Short-circuit evaluation
- Ternary operator
- Prefix/postfix increment/decrement
- Compound assignments
- Multi-level pointers and basic arrays
- `_Bool` type support
- Preprocessor support (includes and macros)
- Basic literals (int, char, string)
- `sizeof` operator (basic support)

#### Platforms
- macOS (Apple Silicon and Intel)
- Linux (Ubuntu 22.04+)
- Arch Linux

#### Command Line Options
- `-o <file>` - Specify output file
- `-v` - Verbose output
- `-d` - Debug mode (emit LLVM IR)
- `-E` - Preprocess only
- `-I <dir>` - Add include directory
- `-D <macro>` - Define macro
- `--version` - Show version information
- `-h, --help` - Show help message

#### Test Suite
- 42 comprehensive test cases
- Automated CI/CD pipeline
- Cross-platform testing