// RUN: %mmoc %s | %run ; if [ $? -eq 6 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Test mixed prefix/postfix ++ and --
int main(){
    int x=3;
    int y=++x;    // x=4, y=4
    int z=x++;    // z=4, x=5
    --x;          // x=4
    x--;          // x=3
    return x + y - z + 3; // 3+4-4+3 = 6
}
