// RUN: %ccomp %s | %run ; if [ $? -eq 10 ]; then echo "PASS"; else echo "FAIL (got $?)"; fi
// For loop with external counter increment pattern
int main(){
    int i=0; for(; i<10; ) { i++; } return i; }
