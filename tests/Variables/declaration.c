// Test variable declarations and usage
// RUN: %mmoc %s -o %t && %t
// RUN: test $? -eq 10

int main() {
    int x = 10;
    return x;
}
