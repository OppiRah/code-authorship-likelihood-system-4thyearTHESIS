// Patricia Pangilinan - Simple Calculator

#include <stdio.h>

int main() {
    double num1, num2;
    char operation;
    char repeat = 'y';

    while (repeat == 'y' || repeat == 'Y') {
        // read the expression
        printf("Enter first number: ");
        scanf("%lf", &num1);
        printf("Enter operator (+ - * /): ");
        scanf(" %c", &operation);
        printf("Enter second number: ");
        scanf("%lf", &num2);
        if (operation == '/' && num2 == 0) {
            puts("Error: division by zero");
        } else {
            switch (operation) {
                case '+':
                printf("Result: %.2f\n", num1 + num2);
                break;
                case '-':
                printf("Result: %.2f\n", num1 - num2);
                break;
                case '*':
                printf("Result: %.2f\n", num1 * num2);
                break;
                case '/':
                printf("Result: %.2f\n", num1 / num2);
                break;
                default:
                puts("Unknown operator");
            }
        }
        printf("Again? (y/n): ");
        scanf(" %c", &repeat);
    }
    return 0;
}
