#!/bin/bash
set +e  # Don't exit on errors
# Comprehensive test suite for mmoc compiler

set -e  # Exit on any error

MMOC_DIR="/Users/yb/git/mmoc"
TEST_DIR="$MMOC_DIR/tests"
COMPILER="$MMOC_DIR/build/ccomp"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Test result tracking
declare -a PASSED_TEST_NAMES
declare -a FAILED_TEST_NAMES

print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}           MMOC COMPILER TEST SUITE${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo
}

print_section() {
    echo -e "${YELLOW}--- $1 ---${NC}"
}

run_test() {
    local test_name="$1"
    local test_file="$2"
    local expected_exit_code="$3"
    local description="$4"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "Test $TOTAL_TESTS: $test_name... "
    
    # Create test file
    echo "$test_file" > "$TEST_DIR/temp_test.c"
    
    # Try to compile (working directory is mmoc root)
    if $COMPILER "$TEST_DIR/temp_test.c" >/dev/null 2>&1; then
        # Compilation succeeded, try to run
        if [ -f "./a.out" ]; then
            ./a.out >/dev/null 2>&1
            actual_exit_code=$?
            if [ $actual_exit_code -eq $expected_exit_code ]; then
                echo -e "${GREEN}PASS${NC} - $description"
                PASSED_TESTS=$((PASSED_TESTS + 1))
                PASSED_TEST_NAMES+=("$test_name")
            else
                echo -e "${RED}FAIL${NC} - Expected exit code $expected_exit_code, got $actual_exit_code"
                FAILED_TESTS=$((FAILED_TESTS + 1))
                FAILED_TEST_NAMES+=("$test_name")
            fi
        else
            echo -e "${RED}FAIL${NC} - No executable created"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            FAILED_TEST_NAMES+=("$test_name")
        fi
    else
        echo -e "${RED}FAIL${NC} - Compilation error"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        FAILED_TEST_NAMES+=("$test_name")
    fi
    
    # Clean up
    rm -f "$TEST_DIR/temp_test.c"
    rm -f "./a.out"
}

run_compile_fail_test() {
    local test_name="$1"
    local test_file="$2"
    local description="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "Test $TOTAL_TESTS: $test_name... "
    
    # Create test file
    echo "$test_file" > "$TEST_DIR/temp_test.c"
    
    # Try to compile - should fail
    if $COMPILER "$TEST_DIR/temp_test.c" >/dev/null 2>&1; then
        echo -e "${RED}FAIL${NC} - Expected compilation to fail but it succeeded"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        FAILED_TEST_NAMES+=("$test_name")
    else
        echo -e "${GREEN}PASS${NC} - $description"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        PASSED_TEST_NAMES+=("$test_name")
    fi
}

# Change to mmoc directory
cd "$MMOC_DIR"

print_header

# ============= BASIC FUNCTIONALITY TESTS =============
print_section "Basic Functionality"

run_test "simple_return" \
"int main() { return 42; }" \
42 \
"Simple return value"

run_test "basic_arithmetic" \
"int main() { return 5 + 3; }" \
8 \
"Basic addition"

run_test "arithmetic_precedence" \
"int main() { return 2 + 3 * 4; }" \
14 \
"Arithmetic precedence (should be 14, not 20)"

run_test "parentheses" \
"int main() { return (2 + 3) * 4; }" \
20 \
"Parentheses in expressions"

# ============= VARIABLE TESTS =============
print_section "Variables"

run_test "variable_declaration" \
"int main() { int x = 10; return x; }" \
10 \
"Variable declaration and usage"

run_test "multiple_variables" \
"int main() { int x = 5; int y = 7; return x + y; }" \
12 \
"Multiple variable declarations"

run_test "variable_assignment" \
"int main() { int x = 5; x = 10; return x; }" \
10 \
"Variable assignment"

# ============= COMPARISON TESTS =============
print_section "Comparison Operators"

run_test "greater_than_true" \
"int main() { return 10 > 5; }" \
1 \
"Greater than (true case)"

run_test "greater_than_false" \
"int main() { return 5 > 10; }" \
0 \
"Greater than (false case)"

run_test "less_than_true" \
"int main() { return 5 < 10; }" \
1 \
"Less than (true case)"

run_test "less_than_false" \
"int main() { return 10 < 5; }" \
0 \
"Less than (false case)"

run_test "equal_true" \
"int main() { return 5 == 5; }" \
1 \
"Equality (true case)"

run_test "equal_false" \
"int main() { return 5 == 10; }" \
0 \
"Equality (false case)"

# ============= FUNCTION TESTS =============
print_section "Functions"

run_test "function_call" \
"int add(int a, int b) { return a + b; } int main() { return add(5, 7); }" \
12 \
"Function definition and call"

run_test "nested_function_calls" \
"int multiply(int x, int y) { return x * y; } int add(int a, int b) { return a + b; } int main() { return add(multiply(2, 3), 4); }" \
10 \
"Nested function calls"

run_test "recursive_function" \
"int factorial(int n) { if (n <= 1) return 1; return n * factorial(n - 1); } int main() { return factorial(4); }" \
24 \
"Recursive function (factorial)"

# ============= CONTROL FLOW TESTS =============
print_section "Control Flow"

run_compile_fail_test "if_statement_simple" \
"int main() { if (1) { return 1; } return 0; }" \
"If statement should fail (not implemented)"

run_compile_fail_test "while_loop" \
"int main() { int i = 0; while (i < 5) { i = i + 1; } return i; }" \
"While loop should fail (not implemented)"

run_compile_fail_test "for_loop" \
"int main() { for (int i = 0; i < 5; i++) { } return 0; }" \
"For loop should fail (not implemented)"

# ============= ADVANCED FEATURES TESTS =============
print_section "Advanced Features"

run_compile_fail_test "arrays" \
"int main() { int arr[5]; arr[0] = 10; return arr[0]; }" \
"Arrays should fail (not implemented)"

run_compile_fail_test "pointers" \
"int main() { int x = 10; int *p = &x; return *p; }" \
"Pointers should fail (not implemented)"

run_compile_fail_test "structs" \
"struct Point { int x; int y; }; int main() { struct Point p; p.x = 5; return p.x; }" \
"Structs should fail (not implemented)"

# ============= ERROR HANDLING TESTS =============
print_section "Error Handling"

run_compile_fail_test "undefined_function" \
"int main() { return undefined_func(); }" \
"Undefined function should fail"

run_compile_fail_test "wrong_arg_count" \
"int add(int a, int b) { return a + b; } int main() { return add(5); }" \
"Wrong argument count should fail"

run_compile_fail_test "syntax_error" \
"int main() { return 5 + ; }" \
"Syntax error should fail"

# ============= CLEAN UP =============
rm -f "$TEST_DIR/temp_test.c" a.out *.ll

# ============= RESULTS =============
echo
print_section "Test Results"
echo -e "Total tests: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -gt 0 ]; then
    echo
    echo -e "${RED}Failed tests:${NC}"
    for test in "${FAILED_TEST_NAMES[@]}"; do
        echo -e "  - $test"
    done
fi

if [ $PASSED_TESTS -gt 0 ]; then
    echo
    echo -e "${GREEN}Passed tests:${NC}"
    for test in "${PASSED_TEST_NAMES[@]}"; do
        echo -e "  - $test"
    done
fi

echo
if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}All tests passed! 🎉${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. See details above.${NC}"
    exit 1
fi
