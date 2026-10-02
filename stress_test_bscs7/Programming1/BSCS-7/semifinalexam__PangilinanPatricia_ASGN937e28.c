// Patricia Pangilinan - Number Analyzer

#include <stdio.h>

int main() {
    int num, temp, digits = 0, digitSum = 0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num < 0) {
        num = -num;
    }

    // count the digits
    temp = num;
    for (; temp; temp = temp / 10) {
        ++digits;
    }
    // sum and reverse
    temp = num;
    for (; temp; temp = temp / 10) {
        digitSum = digitSum + temp % 10;
        reversed = reversed * 10 + temp % 10;
    }
    if (num == 0) {
        digits = 1;
    }
    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digitSum);
    if (reversed == num) {
        puts("It is a palindrome");
    } else {
        puts("It is not a palindrome");
    }
    printf("\n--- Results ---\n");
    return 0;
}
