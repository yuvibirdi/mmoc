// RUN: %mmoc %s -o %t && %t
// Test include directive functionality

#define HELPER_VALUE 42
#include "include_helper.h"

int main() {
    return get_helper_value(); // Should return 42
}
