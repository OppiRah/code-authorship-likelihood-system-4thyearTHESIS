/*
 * Name: Francis Galang
 * Program: Power Function
 */

#include <stdio.h>

long raise_power(int base, int exponent)
{
    long result = 1;
    for (int i = 0; i < exponent; i++)
    {
        result = result * base;
    }
    return result;
}

int main(void)
{
    int base, exponent;
    printf("Base and exponent: ");
    scanf("%d %d", &base, &exponent);
    printf("%d^%d = %ld\n", base, exponent, raise_power(base, exponent));
}
