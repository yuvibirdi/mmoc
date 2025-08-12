// Test while loops
// RUN: %mmoc %s -o %t && %t
// RUN: test $? -eq 5

int main() {
    int x = 0;
    while (x < 5) {
        x = x + 1;
    }
    return x;
}
