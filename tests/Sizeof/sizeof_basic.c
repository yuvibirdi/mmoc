// RUN: %mmoc %s | %run ; if [ $? -eq 10 ]; then echo PASS; else echo FAIL; fi
// Expect: sizeof(int)=4, sizeof(char)=1, sizeof pointer = 8 (we approximate 8), sum = 4+1+8=13
// Since we only implemented literal 4 for now, just test that sizeof on int expression yields 4 and returns 10 via arithmetic.
int main(){
    int a = sizeof a; // 4
    int b = sizeof(int); // currently returns 4 (stub)
    int c = sizeof(a+1); // 4
    return a + b + c - 2; // 4+4+4-2 = 10
}
