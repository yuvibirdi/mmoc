// Test function definitions and calls
// RUN: %mmoc %s -o %t && %t
// RUN: test $? -eq 5

int add(int a, int b) {
    return a + b;
}

int main() {
    return add(2, 3);
}
