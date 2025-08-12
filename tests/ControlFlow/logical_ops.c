// RUN: %mmoc %s | %run ; if [ $? -eq 1 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Test logical operators

int main() {
    int a = 1;
    int b = 0;
    int c = 1;
    
    if (a && c) {  // true && true = true
        if (a || b) {  // true || false = true
            return 1;  // Success
        }
    }
    return 0;  // Failure
}
