/*
 * Name: Mikaela Navarro
 * Program: Simple Calculator
 */

#include <stdio.h>

int main() {
    float num1, num2;
    char operation;
    char repeat='y';

    do {
        printf("Enter first number: ");  // read the expression
        scanf("%f", &num1);
        printf("Enter operator (+ - * /): ");
        scanf(" %c", &operation);
        printf("Enter second number: ");
        scanf("%f", &num2);
        if (operation == '/' && num2==0) {
            printf("Error: division by zero\n");
        } else {
            if (operation == '+') {
                printf("Result: %.2f\n", num1+num2);
            } else if (operation == '-') {
                printf("Result: %.2f\n", num1-num2);
            } else if (operation == '*') {
                printf("Result: %.2f\n", num1*num2);
            } else if (operation == '/') {
                printf("Result: %.2f\n", num1/num2);
            } else {
                printf("Unknown operator\n");
            }
        }
        printf("Again? (y/n): ");
        scanf(" %c", &repeat);
    } while (repeat == 'y'||repeat == 'Y');
    printf("\n--- Results ---\n");
    return 0;
}
