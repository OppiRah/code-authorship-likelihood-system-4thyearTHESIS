#include <stdio.h>

int main() {
    // Activity 2: Sum of digits using a while loop

    int inputNumber;
    int digitSum = 0;

    printf("Enter a positive integer: ");
    scanf("%d", &inputNumber);

    int workingNumber = inputNumber;

    // Extract and sum each digit
    while (workingNumber > 0) {
        int currentDigit = workingNumber % 10;
        digitSum += currentDigit;
        workingNumber = workingNumber / 10;
    }

    printf("\nSum of digits in %d is %d\n", inputNumber, digitSum);

    return 0;
}
