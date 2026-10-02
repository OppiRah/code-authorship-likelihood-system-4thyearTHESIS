#include <stdio.h>

// Activity 1: Calculator using functions

double addNumbers(double a, double b) {
    return a + b;
}

double subtractNumbers(double a, double b) {
    return a - b;
}

double multiplyNumbers(double a, double b) {
    return a * b;
}

int main() {
    double firstValue, secondValue;

    printf("Enter the first number: ");
    scanf("%lf", &firstValue);

    printf("Enter the second number: ");
    scanf("%lf", &secondValue);

    // Call each function and display the results
    printf("\nSum: %.2lf\n", addNumbers(firstValue, secondValue));
    printf("Difference: %.2lf\n", subtractNumbers(firstValue, secondValue));
    printf("Product: %.2lf\n", multiplyNumbers(firstValue, secondValue));

    return 0;
}
