// Basic increment test
// RUN: %ccomp %s | %run ; if [ $? -eq 6 ]; then echo PASS; else echo FAIL; fi
int main(){
    int x=5;
    x++; // x becomes 6
    return x; // expect 6
}
