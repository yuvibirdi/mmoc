// Chained increment test
// RUN: %ccomp %s | %run ; if [ $? -eq 11 ]; then echo PASS; else echo FAIL; fi
int main(){
    int x=5;
    int a=++x; // x=6, a=6
    int b=x++; // b=6, x=7
    // We want deterministic expected 11: use a + (b-1)
    return a + (b-1); // 6 + 5 = 11
}
