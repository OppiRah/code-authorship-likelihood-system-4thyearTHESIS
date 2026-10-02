/*
 * Name: Denise De Guzman
 * Program: Number Analyzer
 */

#include <stdio.h>

int main() {
    int num, temp, digits = 0, digit_sum = 0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);
    temp = num;
    for (; temp > 0; temp /= 10) {
        int remainder=temp % 10;  // last digit
        digits++;
        digit_sum = digit_sum + remainder;
        reversed = reversed * 10 + remainder;
    }
    if (num == 0) {
        digits = 1;
    }

    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digit_sum);
    if (reversed == num) {
        printf("It is a palindrome\n");
    } else {
        printf("It is not a palindrome\n");
    }
    printf("\n--- Results ---\n");
    return 0;
}
