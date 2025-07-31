// RUN: %ccomp %s | %run ; if [ $? -eq 12 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Test continue statement in for loop

int main() {
    int sum = 0;
    for (int i = 1; i <= 6; i = i + 1) {
        if (i == 2 || i == 4) {
            continue;  // Skip when i == 2 or i == 4
        }
        sum = sum + i;
    }
    return sum;  // Should return 12 (1+3+5+6 = 15, but 2+4=6 skipped)
}
