// RUN: %ccomp %s | %run ; if [ $? -eq 9 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Nested ternary and logical mixing
int main(){
    int x=2,y=4,z=9;
    int r = (x<y? (y>z?1: z) : 5);
    return r; // expect 9
}
