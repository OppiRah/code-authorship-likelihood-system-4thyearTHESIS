/*
 * Name: Vanessa Valdez
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
        int remainder = temp%10;
        digits++;
        digit_sum += remainder;
        reversed = reversed * 10 + remainder;
        temp /= 10;
    }
    printf("\n--- Results ---\n");
    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digit_sum);
    printf("%s\n", reversed==num ? "Palindrome" : "Not a palindrome");
    return 0;
}
