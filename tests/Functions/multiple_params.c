// RUN: %ccomp %s | %run ; if [ $? -eq 12 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Test function with multiple parameters

int add(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    return x * y;
}

int main() {
    int result = add(3, 4);  // Should be 7
    result = multiply(result, 2);  // Should be 14
    return result - 2;  // Should return 12
}
