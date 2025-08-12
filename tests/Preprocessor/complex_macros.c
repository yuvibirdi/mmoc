// RUN: %mmoc %s -o %t && %t
// Test complex macro functionality

#define ADD(a, b) ((a) + (b))
#define MAX_INT 2147483647
#define MIN_VAL 1

int main() {
    int result = ADD(MIN_VAL, 4);
    return result; // Should return 5
}
