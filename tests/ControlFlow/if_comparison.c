// Test if statements with comparisons
// RUN: %ccomp %s -o %t && %t
// RUN: test $? -eq 99

int main() {
    int x = 5;
    if (x > 3) return 99;
    return 88;
}
