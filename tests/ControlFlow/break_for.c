// RUN: %ccomp %s | %run ; if [ $? -eq 3 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Test break statement in for loop

int main() {
    int count = 0;
    for (int i = 1; i <= 10; i = i + 1) {
        count = count + 1;
        if (i == 3) {
            break;  // Should exit after 3 iterations
        }
    }
    return count;  // Should return 3
}
