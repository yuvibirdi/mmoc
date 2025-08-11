// RUN: %ccomp %s | %run ; if [ $? -eq 7 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// Basic ternary operator test
int main(){
    int a=1,b=7,c=3;
    return a? b : c; // should be 7
}
