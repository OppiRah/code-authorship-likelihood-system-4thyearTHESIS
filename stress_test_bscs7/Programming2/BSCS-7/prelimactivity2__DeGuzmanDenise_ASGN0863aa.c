/*
 * Name: Denise De Guzman
 * Program: Power Function
 */

#include <stdio.h>

long raise_power(int base, int exponent) {
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
    printf("%d^%d = %ld\n", base, exponent, raise_power(base, exponent));
    return 0;
}
