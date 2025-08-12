// RUN: %mmoc %s | %run ; if [ $? -eq 3 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Short-circuit AND: second side must NOT execute when first is false.

int side_effect() {
    return 99; // Should not be reached
}

int main() {
    int a = 0;
    int b = 3;
    if (a && (b = side_effect())) {
        return 0; // should not happen
    }
    return b; // remains 3 if short-circuit works
}
