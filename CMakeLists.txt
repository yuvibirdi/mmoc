cmake_minimum_required(VERSION 3.20)
project(mmoc VERSION 1.0.0 LANGUAGES C CXX)

# Prevent GCC usage on macOS due to ABI mismatch with Homebrew libraries
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "GCC on macOS is not supported. Homebrew's LLVM and ANTLR4 libraries are built with clang/libc++, causing ABI mismatch with GCC/libstdc++ (std::__1:: vs std::__cxx11::). Please use clang: export CC=clang CXX=clang++")
endif()

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

# On macOS, find the SDK path and add it to compiler flags to find system headers
if(APPLE)
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE MACOSX_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(MACOSX_SDK_PATH)
        # Use CMAKE_CXX_FLAGS for all build types, and also set for C compiler
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -isysroot ${MACOSX_SDK_PATH}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isysroot ${MACOSX_SDK_PATH}")
    endif()
endif()

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Find required packages
include(LLVMSetup)

# Find ANTLR4 runtime flexibly
find_path(ANTLR4_INCLUDE_DIR
    NAMES antlr4-runtime.h
    HINTS $ENV{ANTLR4_INCLUDE} /usr/include /usr/local/include /opt/homebrew/include /opt/local/include
    PATH_SUFFIXES antlr4-runtime
)
find_library(ANTLR4_LIBRARY
    NAMES antlr4-runtime libantlr4-runtime
    HINTS $ENV{ANTLR4_LIB} /usr/lib /usr/local/lib /opt/homebrew/lib /opt/local/lib
)

if(NOT ANTLR4_INCLUDE_DIR OR NOT ANTLR4_LIBRARY)
    message(FATAL_ERROR "Could not locate ANTLR4 runtime (set ANTLR4_INCLUDE / ANTLR4_LIB env vars)")
endif()

# Include directories
include_directories(src)
include_directories(${CMAKE_SOURCE_DIR}/generated)

# Check that ANTLR generated files exist (should be created by setup.sh)
set(ANTLR_FILES
    ${CMAKE_SOURCE_DIR}/generated/CLexer.cpp
    ${CMAKE_SOURCE_DIR}/generated/CParser.cpp
    ${CMAKE_SOURCE_DIR}/generated/CBaseVisitor.cpp
    ${CMAKE_SOURCE_DIR}/generated/CVisitor.cpp
    ${CMAKE_SOURCE_DIR}/generated/CLexer.h
    ${CMAKE_SOURCE_DIR}/generated/CParser.h
    ${CMAKE_SOURCE_DIR}/generated/CBaseVisitor.h
    ${CMAKE_SOURCE_DIR}/generated/CVisitor.h
)

foreach(ANTLR_FILE ${ANTLR_FILES})
    if(NOT EXISTS ${ANTLR_FILE})
        message(FATAL_ERROR "ANTLR generated file ${ANTLR_FILE} not found. Please run setup.sh first to generate parser files.")
    endif()
endforeach()

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

# Integration tests (keep minimal example if needed)
add_test(
    NAME integration_hello_world
    COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:ccomp> ${CMAKE_SOURCE_DIR}/tests/Basic/simple_return.c
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
