// RUN: %ccomp %s | %run ; if [ $? -eq 6 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Test increment operators

int main() {
    int x = 5;
    x++;  // Pre/post increment
    return x;  // Should return 6
}
