/*
 * Name: Bryan Castro
 * Program: Number Analyzer
 */

#include <stdio.h>

int main(void)
{
    int num, temp, digits = 0, digit_sum=0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num < 0)
    {
        // ignore the sign
        num = -num;
    }

    // count the digits
    temp = num;
    while (temp != 0)
    {
        ++digits;
        temp/=10;
    }
    temp=num;
    while (temp != 0)
    {
        digit_sum += temp%10;
        reversed *= 10 + temp % 10;
        temp /= 10;
    }

    printf("\n--- Results ---\n");
    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digit_sum);
    printf("%s\n", reversed == num ? "Palindrome" : "Not a palindrome");
    return 0;
}
