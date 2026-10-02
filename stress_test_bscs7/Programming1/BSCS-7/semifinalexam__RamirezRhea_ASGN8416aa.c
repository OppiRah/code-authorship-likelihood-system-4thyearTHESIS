/*
 * Name: Rhea Ramirez
 * Program: Number Analyzer
 */

#include <stdio.h>

int main(void)
{
    int num, temp, digits = 0, digit_sum = 0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);

    temp = num;
    while (temp > 0)
    {
        int remainder = temp % 10;
        digits += 1;
        digit_sum = digit_sum + remainder;
        reversed=reversed * 10 + remainder;
        temp = temp / 10;
    }
    if (num == 0)
    {
        digits = 1;
    }

    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digit_sum);
    printf("Reversed: %d\n", reversed);
    printf("%s\n", reversed == num ? "Palindrome" : "Not a palindrome");
}
