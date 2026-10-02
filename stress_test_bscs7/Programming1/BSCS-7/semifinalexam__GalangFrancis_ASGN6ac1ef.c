/*
 * Name: Francis Galang
 * Program: Number Analyzer
 */

#include <stdio.h>

int main(void)
{
    int num, temp, digits = 0, digit_sum = 0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num < 0)
    {
        // ignore the sign
        num = -num;
    }

    temp = num;
    for (; temp > 0; temp /= 10)
    {
        // last digit
        int remainder = temp % 10;
        digits++;
        digit_sum = digit_sum + remainder;
        reversed = reversed * 10 + remainder;
    }

    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digit_sum);
    printf("Reversed: %d\n", reversed);
    if (reversed == num)
    {
        printf("It is a palindrome\n");
    }
    else
    {
        printf("It is not a palindrome\n");
    }
}
