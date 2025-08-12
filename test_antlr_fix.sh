#!/bin/bash
# Test script to verify ANTLR compatibility fixes

set -e

echo "Testing ANTLR compatibility fixes..."

# Clean and regenerate
rm -rf generated/* build/ 2>/dev/null || true

# Generate ANTLR files
echo "Generating ANTLR files..."
if command -v antlr4 &> /dev/null; then
    antlr4 -Dlanguage=Cpp -visitor -o generated -Xexact-output-dir grammar/C.g4
elif command -v antlr &> /dev/null; then
    antlr -Dlanguage=Cpp -visitor -o generated -Xexact-output-dir grammar/C.g4
else
    echo "ERROR: No ANTLR tool found"
    exit 1
fi

# Apply patches
echo "Applying patches..."
bash cmake/patch_antlr.sh generated

# Check for problematic patterns
echo "Checking for compatibility issues..."
if grep -r "getTokenNames.*override" generated/ 2>/dev/null; then
    echo "ERROR: Found unpatched getTokenNames override"
    exit 1
fi

if grep -r "std::vector<uint16_t> getSerializedATN" generated/ 2>/dev/null; then
    echo "ERROR: Found unpatched SerializedATN return type"
    exit 1
fi

# Try to build
echo "Testing build..."
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target cgrammar -j

echo "SUCCESS: ANTLR compatibility fixes work correctly!"