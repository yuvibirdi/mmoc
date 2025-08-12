# LLVM-Backed C99 Compiler – Implementation Scope

*Feed this plan to your coding assistant (e.g., Sonnet 4 + GitHub Copilot) to bootstrap a working repository.*

---

## 1 Project Goals

1. Parse **full C99** (extendible to C11).
2. Lower to **LLVM IR** and emit optimized native code.
3. Command-line driver `mmoc <file.c>` producing `a.out`.
4. Clean architecture; all new code in **modern C++20**.

---

## 2 High-Level Milestones

| Phase | Sprint Target | Deliverables |
|-------|--------------|--------------|
|0 |Repo + CI Skeleton|GitHub repo, `main` branch protection, CMake preset, GitHub Actions build matrix (Linux/macOS) |
|1 |Toolchain Setup |Docs + scripts that install LLVM ≥ 17 & ANTLR 4 CPP runtime |
|2 |Grammar & Parser|Add `grammar/C.g4`; generate C++ lexer/parser; smoke-test `int main(){}` |
|3 |AST Layer       |Visitor->AST builder, pretty printer, unit tests |
|4 |Semantic Passes |Symbol table, type checker, typedef disambiguation |
|5 |Code Gen MVP    |IRBuilder code gen for literals, arithmetic, return; produce runnable object |
|6 |Statements      |If/while/for, break/continue, block scopes |
|7 |Functions/Call  |Function prototypes, calling convention, external lib linking |
|8 |Data Types      |Pointers, arrays, structs, unions; VLAs postponed |
|9 |Preprocessor    |Minimal macro expansion using `clang -E` or homemade stub |
|10 |C11 Features   |_Bool, `_Generic`, atomics, threads |
|11 |Optimization   |Enable SSA passes; optional MIR -> ASM benchmarking |
|12 |Release 1.0    |Tag v1.0, docs, Homebrew/Apt formula |

> **Timebox:** 12 × 1-week sprints ≈ 3 months with part-time effort.

---

## 3 Repository Layout

```
.
├── CMakeLists.txt
├── cmake/
│   └── LLVMSetup.cmake
├── grammar/
│   └── C.g4
├── generated/        # git-ignored – ANTLR output
├── src/
│   ├── driver/
│   ├── lexer/        # wrapper around generated lexer
│   ├── parser/
│   ├── ast/
│   ├── sema/
│   ├── codegen/
│   └── utils/
├── test/
│   ├── inputs/
│   └── expected/
└── .github/
    └── workflows/
        └── ci.yml
```

---

## 4 Environment & Tooling

### 4.1 Prerequisites (macOS & Ubuntu)

```bash
# LLVM & Clang ≥ 17
brew install llvm          # macOS
sudo apt-get install llvm clang lld libllvm-dev # Ubuntu

# ANTLR 4 tool & C++ runtime
brew install antlr4-cpp-runtime antlr4-tools    # macOS
sudo apt-get install antlr4-cpp-runtime        # Ubuntu
pip install antlr4-tools                       # grun CLI

# Build utils
brew install cmake ninja cppcheck
sudo apt-get install cmake ninja-build cppcheck
```

Add to shell:
```bash
export CLASSPATH="$(brew --prefix)/opt/antlr4-runtime/libexec/antlr-4.13.2-complete.jar:$CLASSPATH"
alias antlr4='java -jar $(brew --prefix)/opt/antlr4-runtime/libexec/antlr-4.13.2-complete.jar'
```

### 4.2 Generating the Parser

```bash
mkdir -p generated
antlr4 -Dlanguage=Cpp -visitor -o generated grammar/C.g4
```

Integrate in CMake:
```cmake
add_custom_command(
  OUTPUT ${CMAKE_SOURCE_DIR}/generated/CLexer.cpp
  COMMAND antlr4 -Dlanguage=Cpp -visitor -o ${CMAKE_SOURCE_DIR}/generated ${CMAKE_SOURCE_DIR}/grammar/C.g4
  DEPENDS grammar/C.g4
)
add_library(cgrammar STATIC
  generated/CLexer.cpp
  generated/CParser.cpp
  ...)
```

### 4.3 LLVM Linking Snippet

```cmake
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core irreader bitwriter)

target_link_libraries(codegen PRIVATE ${llvm_libs})
```

---

## 5 Coding Conventions

* C++20, `clang-format` profile `Google`.
* Enforce `-Wall -Wextra -Werror -pedantic`.
* Use `std::unique_ptr` for AST nodes.
* One class per header; source in `src/…`.
* Public headers live under `include/` when API stabilizes.

---

## 6 Key Implementation Tasks

### 6.1 AST Node Skeleton

```cpp
namespace ast {
struct Node { virtual ~Node() = default; }; // RTTI via dynamic_cast ok for compiler size
struct Expr : Node { };          // base for expressions
struct IntegerLiteral : Expr {
  long long value;
};
struct ReturnStmt : Node {
  std::unique_ptr<Expr> expr;
};
}
```

### 6.2 Visitor-Based AST Builder

```cpp
class ASTBuilder : public CBaseVisitor {
  antlrcpp::Any visitPrimaryExpression(CParser::PrimaryExpressionContext *ctx) override {
    if (ctx->Constant()) return std::make_unique<ast::IntegerLiteral>(/*…*/);
    // …
  }
};
```

### 6.3 IR Generation (minimal)

```cpp
llvm::LLVMContext ctx;
llvm::Module module("main", ctx);
llvm::IRBuilder<> builder(ctx);

// create `main`
auto *funcType = llvm::FunctionType::get(builder.getInt32Ty(), false);
auto *mainFn   = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module);
auto *entry    = llvm::BasicBlock::Create(ctx, "entry", mainFn);
builder.SetInsertPoint(entry);

builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0));
module.print(llvm::outs(), nullptr);
```

---

## 7 Testing Strategy

1. **Unit**: GoogleTest for lexer, parser, AST utilities.
2. **Integration**: compile sample `.c` files → run produced executables, compare stdout.
3. **Regression**: Each fixed bug adds a minimal test.
4. **Static Analysis**: `cppcheck`, clang-tidy in CI.

---

## 8 CI Pipeline (`.github/workflows/ci.yml`)

```yaml
name: build
on: [push, pull_request]
jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
      - uses: lukka/get-cmake@latest
      - name: Install deps
        run: |
          if [[ "$RUNNER_OS" == "Linux" ]]; then sudo apt-get update && sudo apt-get install -y llvm clang antlr4-cpp-runtime; fi
          if [[ "$RUNNER_OS" == "macOS" ]]; then brew install llvm antlr4-cpp-runtime antlr4-tools; fi
      - name: Configure & Build
        run: cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build --target all -j
      - name: Run tests
        run: ctest --test-dir build --output-on-failure
```

---

## 9 Stretch Goals

* **Precompiled headers** for faster builds.
* **LSP** server exposing AST/IR for IDE integrations.
* **WebAssembly** backend via `wasm32` target.
* **Clang-compatible driver flags** (`-Wall -O2 -S -emit-llvm`).

---

## 10 Reference Resources

* ANTLR C11 grammar – github.com/antlr/grammars-v4/c/C.g4
* ANTLR Mega Tutorial – tomassetti.me/antlr-mega-tutorial
* LLVM “Kaleidoscope” tutorial (C++) – llvm.org/docs/tutorial
* IRBuilder API example – github.com/zilder/llvm-hello-world-example
* Homebrew formula for ANTLR C++ runtime – formulae.brew.sh/formula/antlr4-cpp-runtime
* CMake + LLVM how-to – layle.me/posts/using-llvm-with-cmake

---

### End of Scope Document
