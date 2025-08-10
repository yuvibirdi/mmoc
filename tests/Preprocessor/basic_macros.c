// RUN: %ccomp %s -o %t && %t
// Test basic macro expansion functionality

#define PI 3
#define DOUBLE(x) ((x) * 2)

int main() {
    int radius = PI;
    int doubled = DOUBLE(5);
    return doubled + radius; // Should return 13 (10 + 3)
}
