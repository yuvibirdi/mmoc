#!/bin/bash
# ANTLR4 compatibility patcher script

echo "Patching generated files for compatibility..."

GENERATED_DIR="$1"

if [[ ! -d "$GENERATED_DIR" ]]; then
    echo "Error: Generated directory '$GENERATED_DIR' not found"
    exit 1
fi

# Detect what needs patching by checking the generated files
NEEDS_TOKEN_NAMES_FIX=false
NEEDS_SERIALIZED_ATN_FIX=false

if grep -q "getTokenNames() const override" "$GENERATED_DIR"/*.h 2>/dev/null; then
    NEEDS_TOKEN_NAMES_FIX=true
fi

if grep -q "std::vector<uint16_t> getSerializedATN" "$GENERATED_DIR"/*.h 2>/dev/null; then
    NEEDS_SERIALIZED_ATN_FIX=true
fi

# Fix 1: Remove deprecated getTokenNames override that doesn't exist in newer ANTLR runtime
if [[ "$NEEDS_TOKEN_NAMES_FIX" == "true" ]]; then
    echo "Removing deprecated getTokenNames override..."
    sed -i.bak '/virtual const std::vector<std::string>& getTokenNames() const override/d' "$GENERATED_DIR/CParser.h" 2>/dev/null || true
    sed -i.bak '/virtual const std::vector<std::string>& getTokenNames() const override/d' "$GENERATED_DIR/CLexer.h" 2>/dev/null || true
fi

# Fix 2-6: SerializedATN compatibility fixes
if [[ "$NEEDS_SERIALIZED_ATN_FIX" == "true" ]]; then
    echo "Fixing getSerializedATN return type..."
    sed -i.bak 's/virtual const std::vector<uint16_t> getSerializedATN() const override;/virtual atn::SerializedATNView getSerializedATN() const override;/' "$GENERATED_DIR/CLexer.h" 2>/dev/null || true

    echo "Fixing ATN deserialization..."
    sed -i.bak 's/_atn = deserializer.deserialize(_serializedATN);/_atn = std::move(deserializer.deserialize(atn::SerializedATNView(_serializedATN)));/' "$GENERATED_DIR/CLexer.cpp" 2>/dev/null || true
    sed -i.bak 's/_atn = deserializer.deserialize(_serializedATN);/_atn = std::move(deserializer.deserialize(atn::SerializedATNView(_serializedATN)));/' "$GENERATED_DIR/CParser.cpp" 2>/dev/null || true

    echo "Fixing getSerializedATN implementation..."
    sed -i.bak 's/const std::vector<uint16_t> CLexer::getSerializedATN() const {/atn::SerializedATNView CLexer::getSerializedATN() const {/' "$GENERATED_DIR/CLexer.cpp" 2>/dev/null || true
    sed -i.bak 's/return _serializedATN;/return atn::SerializedATNView(_serializedATN);/' "$GENERATED_DIR/CLexer.cpp" 2>/dev/null || true

    echo "Adding necessary includes..."
    # Only add includes if not already present
    if ! grep -q "SerializedATNView.h" "$GENERATED_DIR/CLexer.cpp" 2>/dev/null; then
        sed -i.bak '1i\
#include "atn/SerializedATNView.h"
' "$GENERATED_DIR/CLexer.cpp" 2>/dev/null || true
    fi
    
    if ! grep -q "SerializedATNView.h" "$GENERATED_DIR/CParser.cpp" 2>/dev/null; then
        sed -i.bak '1i\
#include "atn/SerializedATNView.h"
' "$GENERATED_DIR/CParser.cpp" 2>/dev/null || true
    fi
fi

# Fix 7: Safe string conversion patches (always apply as they're harmless)
echo "Applying string conversion fixes..."
sed -i.bak 's/std::string name = _vocabulary\.getLiteralName(i)/std::string name = std::string(_vocabulary.getLiteralName(i))/' "$GENERATED_DIR/CLexer.cpp" 2>/dev/null || true
sed -i.bak 's/std::string name = _vocabulary\.getLiteralName(i)/std::string name = std::string(_vocabulary.getLiteralName(i))/' "$GENERATED_DIR/CParser.cpp" 2>/dev/null || true

# Clean up backup files
rm -f "$GENERATED_DIR"/*.bak 2>/dev/null || true

echo "Patching complete. Applied compatibility fixes for ANTLR4 runtime version differences."
