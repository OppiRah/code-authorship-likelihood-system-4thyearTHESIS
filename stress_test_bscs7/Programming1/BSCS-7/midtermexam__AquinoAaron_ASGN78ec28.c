// Aaron Aquino - Simple Calculator

#include <stdio.h>

int main() {
    double num1, num2;
    char operation;
    char repeat = 'y';
    while (repeat == 'y' || repeat == 'Y') {
        // read the expression
        scanf("%lf", &num1);
        scanf(" %c", &operation);
        scanf("%lf", &num2);
        if (operation=='/' && num2 == 0) {
            printf("Error: division by zero\n");
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
                printf("Unknown operator\n");
            }
        }
        scanf(" %c", &repeat);
    }
    return 0;
}
