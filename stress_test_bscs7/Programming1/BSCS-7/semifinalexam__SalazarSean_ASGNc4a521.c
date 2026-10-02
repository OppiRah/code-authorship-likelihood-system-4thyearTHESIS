/*
 * Name: Sean Salazar
 * Program: Number Analyzer
 */

#include <stdio.h>

int main() {
    int num, temp, digits = 0, digit_sum = 0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num < 0) {
        num = -num;  // ignore the sign
    }
    temp = num;  // count the digits
    while (temp != 0) {
        ++digits;
        temp = temp/10;
    }
    temp = num;  // sum and reverse
    while (temp != 0) {
        digit_sum = digit_sum + temp%10;
        reversed = reversed * 10 + temp % 10;
        temp = temp / 10;
    }
    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digit_sum);
    printf("Reversed: %d\n", reversed);
    if (reversed == num) {
        puts("It is a palindrome");
    } else {
        puts("It is not a palindrome");
    }
    printf("\n--- Results ---\n");
    return 0;
}
