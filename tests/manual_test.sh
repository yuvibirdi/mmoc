#!/bin/bash

cd /Users/yb/git/mmoc

echo "=== MMOC Compiler Manual Test Summary ==="
echo

# Test 1: Basic return
echo "1. Basic return (42):"
echo 'int main() { return 42; }' > tests/temp.c
./build/ccomp tests/temp.c 2>/dev/null && ./a.out; echo "  Exit code: $?"
echo

# Test 2: Arithmetic
echo "2. Arithmetic (5 + 3):"
echo 'int main() { return 5 + 3; }' > tests/temp.c
./build/ccomp tests/temp.c 2>/dev/null && ./a.out; echo "  Exit code: $?"
echo

# Test 3: Variables
echo "3. Variables (int x = 10):"
echo 'int main() { int x = 10; return x; }' > tests/temp.c
./build/ccomp tests/temp.c 2>/dev/null && ./a.out; echo "  Exit code: $?"
echo

# Test 4: Comparison true
echo "4. Comparison true (5 > 3):"
echo 'int main() { return 5 > 3; }' > tests/temp.c
./build/ccomp tests/temp.c 2>/dev/null && ./a.out; echo "  Exit code: $?"
echo

# Test 5: Comparison false
echo "5. Comparison false (3 > 5):"
echo 'int main() { return 3 > 5; }' > tests/temp.c
./build/ccomp tests/temp.c 2>/dev/null && ./a.out; echo "  Exit code: $?"
echo

# Test 6: Function declaration and call
echo "6. Function calls:"
echo 'int add(int a, int b) { return a + b; } int main() { return add(2, 3); }' > tests/temp.c
./build/ccomp tests/temp.c 2>/dev/null && ./a.out; echo "  Exit code: $?"
echo

# Test 7: Multiple operations
echo "7. Complex expression (2 * 3 + 4):"
echo 'int main() { return 2 * 3 + 4; }' > tests/temp.c
./build/ccomp tests/temp.c 2>/dev/null && ./a.out; echo "  Exit code: $?"
echo

# Test 8: Variable arithmetic
echo "8. Variable arithmetic:"
echo 'int main() { int x = 5; int y = 7; return x + y; }' > tests/temp.c
./build/ccomp tests/temp.c 2>/dev/null && ./a.out; echo "  Exit code: $?"
echo

# Test 9: If statements - NOW WORKING!
echo "9. If statement (true condition):"
echo 'int main() { if (1) return 42; return 0; }' > tests/temp.c
if ./build/ccomp tests/temp.c 2>/dev/null; then
    ./a.out; echo "  Exit code: $?"
else
    echo "  Compilation error"
fi
echo

echo "10. If statement (false condition):"
echo 'int main() { if (0) return 42; return 13; }' > tests/temp.c
if ./build/ccomp tests/temp.c 2>/dev/null; then
    ./a.out; echo "  Exit code: $?"
else
    echo "  Compilation error"
fi
echo

echo "11. If statement with comparison:"
echo 'int main() { int x = 5; if (x > 3) return 99; return 88; }' > tests/temp.c
if ./build/ccomp tests/temp.c 2>/dev/null; then
    ./a.out; echo "  Exit code: $?"
else
    echo "  Compilation error"
fi
echo

# Test what doesn't work yet
echo "=== KNOWN LIMITATIONS ==="
echo "12. While loops (expected compilation error):"
echo 'int main() { while (0) return 1; return 42; }' > tests/temp.c
if ./build/ccomp tests/temp.c 2>/dev/null; then
    ./a.out; echo "  Unexpected success! Exit code: $?"
else
    echo "  Expected compilation error - while loops have parsing issues"
fi
echo

echo "13. Assignment operations (expected compilation error):"
echo 'int main() { int x = 5; x = 10; return x; }' > tests/temp.c
if ./build/ccomp tests/temp.c 2>/dev/null; then
    ./a.out; echo "  Unexpected success! Exit code: $?"
else
    echo "  Expected compilation error - assignment operations not implemented"
fi

rm -f tests/temp.c a.out
