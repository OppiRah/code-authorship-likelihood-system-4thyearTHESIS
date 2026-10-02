// Faith Flores - Number Analyzer

#include <stdio.h>

int main() {
    int num, temp, digits = 0, digitSum = 0, reversed = 0;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num < 0) {
        num = -num;
    }
    temp = num;
    for (; temp != 0; temp = temp / 10) {
        digits += 1;
    }
    temp = num;
    for (; temp != 0; temp=temp / 10) {
        digitSum = digitSum + temp % 10;
        reversed = reversed * 10 + temp % 10;
    }

    printf("Digits: %d\n", digits);
    printf("Sum of digits: %d\n", digitSum);
    printf("Reversed: %d\n", reversed);
    if (reversed == num) {
        printf("It is a palindrome\n");
    } else {
        printf("It is not a palindrome\n");
    }
    return 0;
}
