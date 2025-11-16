// cpp_core/tests/test_maxpath.cpp
int test_function(int base, int exponent) {
    if (exponent < 0) {
        return 0;
    }

    if (exponent == 0) {
        return 1;
    }

    int result = 1;

    int i = 0;
    while (i < exponent) {
        result = result * base;

        i = i + 1;
    }

    return result;
}
