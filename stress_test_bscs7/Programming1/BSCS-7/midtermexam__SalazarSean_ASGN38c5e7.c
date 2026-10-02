/*
 * Name: Sean Salazar
 * Program: Simple Calculator
 */

#include <stdio.h>

int main() {
    double num1, num2, result;
    char operation;
    char repeat = 'y';

    do {
        printf("Enter first number: ");  // read the expression
        scanf("%lf", &num1);
        printf("Enter operator (+ - * /): ");
        scanf(" %c", &operation);
        printf("Enter second number: ");
        scanf("%lf", &num2);
        if (operation=='+') {
            result = num1 + num2;
            printf("Result: %.2f\n", result);
        } else if (operation == '-') {
            result = num1 - num2;
            printf("Result: %.2f\n", result);
        } else if (operation == '*') {
            result=num1 * num2;
            printf("Result: %.2f\n", result);
        } else if (operation == '/') {
            if (num2 != 0) {
                result = num1 / num2;
                printf("Result: %.2f\n", result);
            } else {
                puts("Cannot divide by zero");
            }
        } else {
            puts("Unknown operator");
        }
        printf("Again? (y/n): ");
        scanf(" %c", &repeat);
    } while (repeat == 'y' || repeat == 'Y');
    return 0;
}
