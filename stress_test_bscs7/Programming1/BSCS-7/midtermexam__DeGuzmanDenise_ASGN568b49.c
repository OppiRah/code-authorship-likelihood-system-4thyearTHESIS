/*
 * Name: Denise De Guzman
 * Program: Simple Calculator
 */

#include <stdio.h>

int main() {
    float num1, num2, result;
    char operation;

    printf("Enter first number: ");
    scanf("%f", &num1);
    printf("Enter operator (+ - * /): ");
    scanf(" %c", &operation);
    printf("Enter second number: ");
    scanf("%f", &num2);
    if (operation == '/' && num2 == 0) {
        printf("Error: division by zero\n");
    } else {
        if (operation=='+') {
            result = num1 + num2;
            printf("Result: %.2f\n", result);
        } else if (operation == '-') {
            result = num1 - num2;
            printf("Result: %.2f\n", result);
        } else if (operation == '*') {
            result = num1 * num2;
            printf("Result: %.2f\n", result);
        } else if (operation == '/') {
            result = num1 / num2;
            printf("Result: %.2f\n", result);
        } else {
            printf("Unknown operator\n");
        }
    }
    return 0;
}
