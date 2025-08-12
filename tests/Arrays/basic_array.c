// RUN: %mmoc %s | %run ; if [ $? -eq 42 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Simple array test

int main() {
    int arr[3];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 12;
    return arr[0] + arr[1] + arr[2];  // Should return 42
}
