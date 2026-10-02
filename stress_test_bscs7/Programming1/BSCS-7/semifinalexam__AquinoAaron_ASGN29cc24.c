// Aaron Aquino - Number Analyzer

#include <stdio.h>

int main() {
    int num, temp, digits = 0, digitSum = 0, reversed = 0;
    scanf("%d", &num);

    temp = num;
    while (temp > 0) {
        int remainder = temp % 10;
        ++digits;
        digitSum = digitSum + remainder;
        reversed = reversed * 10 + remainder;
        temp = temp / 10;
    }

    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digitSum);
    if (reversed == num) {
        printf("It is a palindrome\n");
    } else {
        printf("It is not a palindrome\n");
    }
    return 0;
}
