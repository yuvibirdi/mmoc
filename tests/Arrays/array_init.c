// RUN: %ccomp %s | %run ; if [ $? -eq 20 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Array with initialization

int main() {
    int arr[4] = {5, 10, 3, 2};
    return arr[0] + arr[1] + arr[2] + arr[3];  // Should return 20
}
