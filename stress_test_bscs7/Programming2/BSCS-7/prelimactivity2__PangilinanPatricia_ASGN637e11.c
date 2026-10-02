// Patricia Pangilinan - Power Function

#include <stdio.h>

long raisePower(int base, int exponent) {
    long result = 1;
    while (exponent > 0) {
        result = result * base;
        exponent--;
    }
    return result;
}

int main() {
    int base, exponent;
    printf("Base and exponent: ");
    scanf("%d %d", &base, &exponent);
    printf("%d^%d = %ld\n", base, exponent, raisePower(base, exponent));
    return 0;
}
