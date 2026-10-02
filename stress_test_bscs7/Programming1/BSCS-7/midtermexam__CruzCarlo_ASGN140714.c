// Carlo Cruz - Simple Calculator

#include <stdio.h>

int main() {
    float num1, num2, result;
    char operation;

    // read the expression
    printf("Enter first number: ");
    scanf("%f", &num1);
    printf("Enter operator (+ - * /): ");
    scanf(" %c", &operation);
    printf("Enter second number: ");
    scanf("%f", &num2);
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
        if (num2 == 0) {
            puts("Cannot divide by zero");
        } else {
            result = num1/num2;
            printf("Result: %.2f\n", result);
        }
        break;
        default:
        puts("Unknown operator");
    }
    printf("\n--- Results ---\n");
    return 0;
}
