// RUN: %ccomp %s | %run ; if [ $? -eq 10 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// For loop with different increment

int main() {
    int count = 0;
    for (int i = 0; i < 10; i = i + 2) {
        count = count + 1;
    }
    return count;  // Should return 5 (i: 0,2,4,6,8)
}
