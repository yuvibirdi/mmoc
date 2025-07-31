// RUN: %ccomp %s | %run ; if [ $? -eq 15 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Test break statement in while loop

int main() {
    int sum = 0;
    int i = 1;
    while (i <= 10) {
        if (i == 6) {
            break;  // Should exit the loop when i == 6
        }
        sum = sum + i;
        i = i + 1;
    }
    return sum;  // Should return 15 (1+2+3+4+5)
}
