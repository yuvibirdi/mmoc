// RUN: %mmoc %s | %run ; if [ $? -eq 1 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Temporary regression guard
int main(){ return 1; }
