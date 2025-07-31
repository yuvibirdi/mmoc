// Test variable assignment
// RUN: %ccomp %s -o %t && %t
// RUN: test $? -eq 15

int main() {
    int x = 5;
    x = 15;
    return x;
}
