#include <stdio.h>

// Activity 2: Prime number checker function

int isPrimeNumber(int number) {
    if (number < 2) {
        return 0;
    }

    for (int divisor = 2; divisor * divisor <= number; divisor++) {
        if (number % divisor == 0) {
            return 0;
        }
    }

    return 1;
}

int main() {
    int inputValue;

    printf("Enter a number to check: ");
    scanf("%d", &inputValue);

    if (isPrimeNumber(inputValue)) {
        printf("\n%d is a prime number.\n", inputValue);
    } else {
        printf("\n%d is not a prime number.\n", inputValue);
    }

    return 0;
}
