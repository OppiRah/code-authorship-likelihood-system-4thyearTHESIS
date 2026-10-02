/*
 * Name: Mikaela Navarro
 * Program: Number Analyzer
 */

#include <stdio.h>

int main() {
    int num, temp, digits = 0, digit_sum = 0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);

    temp = num;
    for (; temp!=0; temp = temp / 10) {
        digits++;
    }
    temp = num;  // sum and reverse
    for (; temp != 0; temp = temp / 10) {
        digit_sum = digit_sum + temp % 10;
        reversed = reversed * 10 + temp % 10;
    }
    if (num==0) {
        digits = 1;
    }
    printf("Digits: %d\nSum of digits: %d\nReversed: %d\n%s\n", digits, digit_sum, reversed, reversed == num ? "Palindrome" : "Not a palindrome");
    return 0;
}
