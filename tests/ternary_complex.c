int max(int a, int b) {
    return (a > b) ? a : b;
}

int main() {
    int x = 5;
    int y = 10;
    int result = max(x, y);
    
    // Test nested ternary
    int z = (x > y) ? (x > 0 ? x : -x) : (y > 0 ? y : -y);
    
    return result + z;
}
