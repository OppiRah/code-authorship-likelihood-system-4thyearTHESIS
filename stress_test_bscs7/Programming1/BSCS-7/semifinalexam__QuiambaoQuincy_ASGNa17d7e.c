/*
 * Name: Quincy Quiambao
 * Program: Number Analyzer
 */

#include <stdio.h>

int main(void)
{
    int num, temp, digits = 0, digit_sum = 0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);

    temp = num;
    for (; temp != 0; temp = temp / 10)
    {
        digits += 1;
    }
    temp = num;
    for (; temp != 0; temp = temp / 10)
    {
        digit_sum = digit_sum + temp % 10;
        reversed = reversed * 10+temp % 10;
    }
    if (num == 0)
    {
        digits = 1;
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
