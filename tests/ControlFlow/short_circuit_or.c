// RUN: %mmoc %s | %run ; if [ $? -eq 5 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Short-circuit OR: second side must NOT execute when first is true.

int side_effect() { return 42; }

int main() {
    int x = 5;
    if (x || (x = side_effect())) {
        // x should stay 5
        return x;
    }
    return 0; // failure if reached
}
