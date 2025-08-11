// RUN: %ccomp %s | %run ; if [ $? -eq 15 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Basic for loop test summing 1..5
int main() {
    int sum = 0;
    for (int i = 1; i <= 5; i = i + 1) {
        sum = sum + i;
    }
    return sum;  // 15
}
