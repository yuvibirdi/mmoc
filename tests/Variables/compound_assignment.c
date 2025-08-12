// RUN: %mmoc %s | %run ; if [ $? -eq 25 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Test compound assignment operators

int main() {
    int x = 10;
    x += 5;    // x = 15
    x -= 3;    // x = 12
    x *= 2;    // x = 24
    x /= 2;    // x = 12
    x %= 5;    // x = 2
    
    x += 23;   // x = 25
    return x;
}
