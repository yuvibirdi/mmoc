#!/bin/bash

# Fix ANTLR .as<>() calls to use std::any_cast<>()
# Also fix return statements to wrap in std::any()

cd /Users/yb/git/mmoc

# Create a backup
cp src/parser/ASTBuilder.cpp src/parser/ASTBuilder.cpp.backup

# Fix the .as<> calls - handle nested parentheses properly
perl -i -pe 's/(\w+\([^)]*\))\.as<([^>]+)>\(\)/std::any_cast<$2>($1)/g' src/parser/ASTBuilder.cpp

# Fix simple return statements that need std::any wrapping
perl -i -pe 's/return (std::make_unique<[^>]+>[^;]+);/return std::any($1);/g' src/parser/ASTBuilder.cpp
perl -i -pe 's/return (std::unique_ptr<[^>]+>\([^;]+\));/return std::any($1);/g' src/parser/ASTBuilder.cpp

echo "Fixed ANTLR API compatibility issues"
