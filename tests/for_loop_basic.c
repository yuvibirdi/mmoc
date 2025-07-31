// RUN: %ccomp %s | %run ; if [ $? -eq 15 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Basic for loop test

int main() {
    int sum = 0;
    for (int i = 1; i <= 5; i = i + 1) {
        sum = sum + i;
    }
    return sum;  // Should return 15 (1+2+3+4+5)
}
