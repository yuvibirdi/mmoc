#!/bin/bash

# Setup script for MMOC compiler dependencies

set -e

echo "MMOC Compiler Setup Script"
echo "=========================="

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Check if it's Arch Linux
    if [[ -f /etc/arch-release ]] || command -v pacman &> /dev/null; then
        OS="arch"
    else
        OS="linux"
    fi
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
        antlr \
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
        cmake \
        ninja-build \
        pkg-config \
        cppcheck \
        build-essential \
        openjdk-17-jre-headless \
        curl \
        wget
    
    echo "Creating symlinks..."
    sudo ln -sf /usr/bin/llvm-config-17 /usr/bin/llvm-config || true
    sudo ln -sf /usr/bin/clang-17 /usr/bin/clang || true
    sudo ln -sf /usr/bin/clang++-17 /usr/bin/clang++ || true
    
    echo "Installing ANTLR4 tool..."
    # Install ANTLR4 tool since Ubuntu doesn't have antlr4 package in standard repos
    cd /tmp
    wget https://www.antlr.org/download/antlr-4.13.1-complete.jar
    sudo mkdir -p /usr/local/lib
    sudo mv antlr-4.13.1-complete.jar /usr/local/lib/
    echo '#!/bin/bash' | sudo tee /usr/local/bin/antlr4
    echo 'java -jar /usr/local/lib/antlr-4.13.1-complete.jar "$@"' | sudo tee -a /usr/local/bin/antlr4
    sudo chmod +x /usr/local/bin/antlr4
    cd - > /dev/null

elif [[ "$OS" == "arch" ]]; then
    echo "Installing dependencies via pacman..."
    
    # Check if we need sudo (not in container)
    if [[ $EUID -eq 0 ]]; then
        # Running as root (e.g., in container)
        pacman -Sy --noconfirm --needed \
            base-devel \
            llvm \
            clang \
            lld \
            cmake \
            ninja \
            python \
            git \
            cppcheck \
            curl \
            jdk-openjdk \
            antlr4-runtime \
            antlr4
    else
        # Running as regular user
        sudo pacman -Sy --noconfirm --needed \
            base-devel \
            llvm \
            clang \
            lld \
            cmake \
            ninja \
            python \
            git \
            cppcheck \
            curl \
            jdk-openjdk \
            antlr4-runtime \
            antlr4
    fi

fi

echo ""
echo "Generating ANTLR parser files..."
echo "================================"

# Create generated directory
mkdir -p generated/

# Find ANTLR tool (antlr4 on Linux, antlr on macOS)
ANTLR_TOOL=""
if command -v antlr4 &> /dev/null; then
    ANTLR_TOOL="antlr4"
elif command -v antlr &> /dev/null; then
    ANTLR_TOOL="antlr"
else
    echo "Error: Could not find ANTLR tool (antlr4 or antlr)"
    exit 1
fi

echo "Using ANTLR tool: $ANTLR_TOOL"

# Generate ANTLR files
$ANTLR_TOOL -Dlanguage=Cpp -visitor -o generated -Xexact-output-dir grammar/C.g4

# Apply compatibility patches
if [[ -f cmake/patch_antlr.sh ]]; then
    echo "Applying ANTLR compatibility patches..."
    bash cmake/patch_antlr.sh generated
else
    echo "Warning: patch_antlr.sh not found, skipping patches"
fi

echo "ANTLR parser files generated successfully!"
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
echo "   python3 tests/test_runner.py"
