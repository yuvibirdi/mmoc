cmake_minimum_required(VERSION 3.20)
project(mmoc VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard and compiler flags
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags (LLVM-specific warnings will be suppressed via system includes)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Find required packages
include(LLVMSetup)

# Manually set ANTLR4 paths for macOS
set(ANTLR4_INCLUDE_DIR "/opt/homebrew/include/antlr4-runtime")
set(ANTLR4_LIBRARY "/opt/homebrew/lib/libantlr4-runtime.dylib")

# Include directories
include_directories(src)
include_directories(${CMAKE_SOURCE_DIR}/generated)

# Create generated directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/generated)

# Generate parser files
add_custom_command(
    OUTPUT 
        ${CMAKE_SOURCE_DIR}/generated/CLexer.cpp
        ${CMAKE_SOURCE_DIR}/generated/CParser.cpp
        ${CMAKE_SOURCE_DIR}/generated/CBaseVisitor.cpp
        ${CMAKE_SOURCE_DIR}/generated/CVisitor.cpp
        ${CMAKE_SOURCE_DIR}/generated/CLexer.h
        ${CMAKE_SOURCE_DIR}/generated/CParser.h
        ${CMAKE_SOURCE_DIR}/generated/CBaseVisitor.h
        ${CMAKE_SOURCE_DIR}/generated/CVisitor.h
    COMMAND antlr -Dlanguage=Cpp -visitor -o ${CMAKE_SOURCE_DIR}/generated ${CMAKE_SOURCE_DIR}/grammar/C.g4
    DEPENDS grammar/C.g4
    COMMENT "Generating ANTLR4 parser files"
)

# Grammar library
add_library(cgrammar STATIC
    generated/CLexer.cpp
    generated/CParser.cpp
    generated/CBaseVisitor.cpp
    generated/CVisitor.cpp
)
target_include_directories(cgrammar PUBLIC ${CMAKE_SOURCE_DIR}/generated)
target_link_libraries(cgrammar PUBLIC ${ANTLR4_LIBRARY})
target_include_directories(cgrammar PUBLIC ${ANTLR4_INCLUDE_DIR})

# Suppress warnings for ANTLR generated code
target_compile_options(cgrammar PRIVATE -Wno-unused-parameter)

# AST library
add_library(cast STATIC
    src/ast/Node.cpp
    src/ast/Expr.cpp
    src/ast/Stmt.cpp
)
target_include_directories(cast PUBLIC src)

# Semantic analysis library
add_library(csema STATIC
    src/sema/SymbolTable.cpp
    src/sema/TypeChecker.cpp
)
target_include_directories(csema PUBLIC src)
target_link_libraries(csema PUBLIC cast)

# Preprocessor library
add_library(cpreprocessor STATIC
    src/preprocessor/Preprocessor.cpp
)
target_include_directories(cpreprocessor PUBLIC src)

# Code generation library
add_library(ccodegen STATIC
    src/codegen/IRGenerator.cpp
)
target_include_directories(ccodegen PUBLIC src)
target_link_libraries(ccodegen PUBLIC cast)

# Parser library
add_library(cparser STATIC
    src/parser/ASTBuilder.cpp
)
target_include_directories(cparser PUBLIC src)
target_link_libraries(cparser PUBLIC cgrammar cast)

# Utils library
add_library(cutils STATIC
    src/utils/Error.cpp
)
target_include_directories(cutils PUBLIC src)

# Main compiler executable
add_executable(ccomp
    src/driver/main.cpp
    src/driver/Driver.cpp
)
target_link_libraries(ccomp PRIVATE 
    cparser 
    csema 
    ccodegen 
    cpreprocessor
    cutils
)
# Link LLVM libraries separately to avoid duplicates
target_link_libraries(ccomp PRIVATE ${LLVM_LIBS})

# Enable testing
enable_testing()

# Find GoogleTest (if available)
find_package(GTest QUIET)
if(GTest_FOUND)
    add_executable(tests
        test/unit/test_lexer.cpp
        test/unit/test_parser.cpp
        test/unit/test_ast.cpp
    )
    target_link_libraries(tests PRIVATE 
        cparser 
        cast 
        cutils
        GTest::gtest 
        GTest::gtest_main
    )
    add_test(NAME unit_tests COMMAND tests)
endif()

# Integration tests
add_test(
    NAME integration_hello_world
    COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:ccomp> ${CMAKE_SOURCE_DIR}/test/inputs/hello.c
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
