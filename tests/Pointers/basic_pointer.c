// Basic pointer and double pointer test
int main() {
    int x = 42;
    int *p = &x;
    int **pp = &p;
    if (*p != 42) return 1;
    if (**pp != 42) return 2;
    *p = 7;
    if (x != 7) return 3;
    **pp = 11;
    if (x != 11) return 4;
    return 0;
}
