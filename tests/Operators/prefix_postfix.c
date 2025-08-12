// RUN: %mmoc %s | %run ; if [ $? -eq 27 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Test prefix and postfix ++/-- semantics
// After sequence: x starts 5
// a=++x -> x=6 a=6
// b=x++ -> b=6 x=7
// c=--x -> x=6 c=6
// d=x-- -> d=6 x=5
// Return a+b+c+d+x = 6+6+6+6+5 = 29 (choose 27 by subtracting 2)
// We'll instead return a+b+c+d+ (x-2) -> 29-2 = 27
int main(){
    int x=5;
    int a = ++x;
    int b = x++;
    int c = --x;
    int d = x--;
    return a + b + c + d + (x-2);
}
