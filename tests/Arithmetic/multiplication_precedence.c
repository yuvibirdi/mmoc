// Test multiplication with precedence
// RUN: %mmoc %s -o %t && %t
// RUN: test $? -eq 10

int main() {
    return 2 * 3 + 4;
}
