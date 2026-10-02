/*
 * Name: Daniel Estrada
 * Program: Number Analyzer
 */

#include <stdio.h>

int main(void)
{
    int num, temp, digits = 0, digit_sum = 0, reversed = 0;
    scanf("%d", &num);
    if (num < 0)
    {
        // ignore the sign
        num=-num;
    }

    temp = num;
    for (; temp != 0; temp = temp / 10)
    {
        digits += 1;
    }
    // sum and reverse
    temp = num;
    for (; temp != 0; temp = temp / 10)
    {
        digit_sum = digit_sum + temp % 10;
        reversed = reversed * 10 + temp%10;
    }

    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digit_sum);
    printf("Reversed: %d\n", reversed);
    if (reversed == num)
    {
        puts("It is a palindrome");
    }
    else
    {
        puts("It is not a palindrome");
    }
    return 0;
}
