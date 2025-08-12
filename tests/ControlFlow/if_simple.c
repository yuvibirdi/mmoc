// Test if statements - should work now
// RUN: %mmoc %s -o %t && %t
// RUN: test $? -eq 42

int main() {
    if (1) return 42;
    return 0;
}
