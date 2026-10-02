// Carlo Cruz - Number Analyzer

#include <stdio.h>

int main() {
    int num, temp, digits = 0, digitSum = 0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num < 0) {
        num = -num;  // ignore the sign
    }
    // count the digits
    temp = num;
    for (; temp != 0; temp=temp / 10) {
        digits++;
    }
    temp = num;
    for (; temp != 0; temp = temp / 10) {
        digitSum = digitSum + temp % 10;
        reversed = reversed * 10 + temp % 10;
    }

    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digitSum);
    printf("Reversed: %d\n", reversed);
    printf("%s\n", reversed == num ? "Palindrome" : "Not a palindrome");
    return 0;
}
