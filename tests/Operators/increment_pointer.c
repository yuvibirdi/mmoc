// Pointer increment test (no scaling yet, just ensuring ++ applies)
// RUN: %mmoc %s | %run ; if [ $? -eq 3 ]; then echo PASS; else echo FAIL; fi
int main(){
    int x=2;
    int *p=&x;
    ++*p; // x becomes 3
    return x; // expect 3
}
