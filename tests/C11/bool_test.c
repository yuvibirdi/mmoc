_Bool test_bool() {
    _Bool result = 1;
    _Bool zero = 0;
    
    if (result) {
        return result;
    } else {
        return zero;
    }
}

int main() {
    _Bool success = test_bool();
    _Bool failure = !success;
    
    return success ? 0 : 1;
}
