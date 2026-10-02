/*
 * Name: Warren Villanueva
 * Program: Simple Calculator
 */

#include <stdio.h>

int main() {
    double num1, num2;
    char operation;
    printf("Enter first number: ");  // read the expression
    scanf("%lf", &num1);
    printf("Enter operator (+ - * /): ");
    scanf(" %c", &operation);
    printf("Enter second number: ");
    scanf("%lf", &num2);
    if (operation == '/'&&num2 == 0) {
        printf("Error: division by zero\n");
    } else {
        if (operation == '+') {
            printf("Result: %.2f\n", num1 + num2);
        } else if (operation == '-') {
            printf("Result: %.2f\n", num1-num2);
        } else if (operation == '*') {
            printf("Result: %.2f\n", num1 * num2);
        } else if (operation == '/') {
            printf("Result: %.2f\n", num1 / num2);
        } else {
            printf("Unknown operator\n");
        }
    }
}
