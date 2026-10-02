// Erika Fajardo - Simple Calculator

#include <stdio.h>

int main() {
    float num1, num2, result;
    char operation;
    char repeat = 'y';
    do {
        printf("Enter first number: ");
        scanf("%f", &num1);
        printf("Enter operator (+ - * /): ");
        scanf(" %c", &operation);
        printf("Enter second number: ");
        scanf("%f", &num2);
        if (operation == '/' && num2 == 0) {
            printf("Error: division by zero\n");
        } else {
            switch (operation) {
                case '+':
                result = num1 + num2;
                printf("Result: %.2f\n", result);
                break;
                case '-':
                result = num1 - num2;
                printf("Result: %.2f\n", result);
                break;
                case '*':
                result = num1 * num2;
                printf("Result: %.2f\n", result);
                break;
                case '/':
                result = num1 / num2;
                printf("Result: %.2f\n", result);
                break;
                default:
                printf("Unknown operator\n");
            }
        }
        printf("Again? (y/n): ");
        scanf(" %c", &repeat);
    } while (repeat == 'y'||repeat == 'Y');
    return 0;
}
