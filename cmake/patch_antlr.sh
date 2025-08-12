#!/bin/bash
# ANTLR4 compatibility patcher script

echo "Patching generated files for compatibility..."

# Only apply safe string conversion patches that are harmless across versions
sed -i.bak 's/std::string name = _vocabulary\.getLiteralName(i)/std::string name = std::string(_vocabulary.getLiteralName(i))/' "$1/CLexer.cpp" 2>/dev/null || true
sed -i.bak 's/std::string name = _vocabulary\.getLiteralName(i)/std::string name = std::string(_vocabulary.getLiteralName(i))/' "$1/CParser.cpp" 2>/dev/null || true

echo "Patching complete."
