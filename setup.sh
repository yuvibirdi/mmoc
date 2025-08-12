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
        llvm-16 \
        llvm-16-dev \
        clang-16 \
        lld-16 \
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
    sudo ln -sf /usr/bin/llvm-config-16 /usr/bin/llvm-config || true
    sudo ln -sf /usr/bin/clang-16 /usr/bin/clang || true
    sudo ln -sf /usr/bin/clang++-16 /usr/bin/clang++ || true

    # Download and install a compatible ANTLR4 tool version
    echo "Installing ANTLR4 tool..."
    ANTLR_VERSION="4.13.2"
    ANTLR_JAR="/usr/local/lib/antlr-${ANTLR_VERSION}-complete.jar"
    
    if [[ ! -f "$ANTLR_JAR" ]]; then
        sudo mkdir -p /usr/local/lib
        sudo curl -o "$ANTLR_JAR" "https://www.antlr.org/download/antlr-${ANTLR_VERSION}-complete.jar"
    fi
    
    # Create antlr4 wrapper script
    sudo tee /usr/local/bin/antlr4 > /dev/null << EOF
#!/bin/bash
java -jar "$ANTLR_JAR" "\$@"
EOF
    sudo chmod +x /usr/local/bin/antlr4
    
    echo "ANTLR4 tool version $ANTLR_VERSION installed."

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
# Clean any existing generated files to avoid stale artifacts across versions
rm -f generated/*.cpp generated/*.h generated/*.interp generated/*.tokens 2>/dev/null || true

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
