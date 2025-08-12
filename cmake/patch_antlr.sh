#!/bin/bash
# ANTLR4 compatibility patcher script

echo "Patching generated files for compatibility..."

# Fix getTokenNames override issue
sed -i.bak 's/getTokenNames() const override/getTokenNames() const/' "$1/CLexer.h" 2>/dev/null || true
sed -i.bak 's/getTokenNames() const override/getTokenNames() const/' "$1/CParser.h" 2>/dev/null || true

# Fix SerializedATNView type mismatch
sed -i.bak 's/const std::vector<uint16_t>/atn::SerializedATNView/' "$1/CLexer.h" 2>/dev/null || true

# Fix string_view to string conversion
sed -i.bak 's/std::string name = _vocabulary\.getLiteralName(i)/std::string name = std::string(_vocabulary.getLiteralName(i))/' "$1/CLexer.cpp" 2>/dev/null || true
sed -i.bak 's/std::string name = _vocabulary\.getLiteralName(i)/std::string name = std::string(_vocabulary.getLiteralName(i))/' "$1/CParser.cpp" 2>/dev/null || true

# Fix ATN deserializer assignment
sed -i.bak 's/_atn = deserializer\.deserialize(_serializedATN)/_atn = std::move(deserializer.deserialize(atn::SerializedATNView(_serializedATN)))/' "$1/CLexer.cpp" 2>/dev/null || true
sed -i.bak 's/_atn = deserializer\.deserialize(_serializedATN)/_atn = std::move(deserializer.deserialize(atn::SerializedATNView(_serializedATN)))/' "$1/CParser.cpp" 2>/dev/null || true

echo "Patching complete."
