#!/bin/bash

# Setup script for MMOC compiler dependencies

set -e

echo "MMOC Compiler Setup Script"
echo "=========================="

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

echo "Detected OS: $OS"

# Install dependencies
if [[ "$OS" == "macos" ]]; then
    echo "Installing dependencies via Homebrew..."
    
    # Check if brew is installed
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Please install Homebrew first:"
        echo "https://brew.sh"
        exit 1
    fi
    
    brew install \
        llvm \
        antlr4 \
        antlr4-cpp-runtime \
        cmake \
        ninja \
        pkg-config \
        cppcheck
    
    echo "Setting up environment..."
    LLVM_PATH="$(brew --prefix llvm)"
    echo "Add to your shell profile:"
    echo "export PATH=\"$LLVM_PATH/bin:\$PATH\""
    echo "export LDFLAGS=\"-L$LLVM_PATH/lib\""
    echo "export CPPFLAGS=\"-I$LLVM_PATH/include\""
    
elif [[ "$OS" == "linux" ]]; then
    echo "Installing dependencies via apt..."
    
    sudo apt-get update
    sudo apt-get install -y \
        llvm-17 \
        llvm-17-dev \
        clang-17 \
        lld-17 \
        libantlr4-runtime-dev \
        antlr4 \
        cmake \
        ninja-build \
        pkg-config \
        cppcheck \
        build-essential
    
    echo "Creating symlinks..."
    sudo ln -sf /usr/bin/llvm-config-17 /usr/bin/llvm-config || true
    sudo ln -sf /usr/bin/clang-17 /usr/bin/clang || true
    sudo ln -sf /usr/bin/clang++-17 /usr/bin/clang++ || true
fi

echo ""
echo "Dependencies installed successfully!"
echo ""
echo "Next steps:"
echo "1. Configure the project:"
echo "   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release"
echo ""
echo "2. Build the compiler:"
echo "   cmake --build build --target all -j"
echo ""
echo "3. Test the installation:"
echo "   ./build/ccomp test/inputs/hello.c && ./a.out"
