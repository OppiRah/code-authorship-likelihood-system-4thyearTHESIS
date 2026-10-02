/*
 * Name: Czarina Dizon
 * Program: Simple Calculator
 */

#include <stdio.h>

int main(void)
{
    double num1, num2;
    char operation;
    char repeat = 'y';

    do
    {
        // read the expression
        printf("Enter first number: ");
        scanf("%lf", &num1);
        printf("Enter operator (+ - * /): ");
        scanf(" %c", &operation);
        printf("Enter second number: ");
        scanf("%lf", &num2);
        switch (operation)
        {
            case '+':
            printf("Result: %.2f\n", num1 + num2);
            break;
            case '-':
            printf("Result: %.2f\n", num1 - num2);
            break;
            case '*':
            printf("Result: %.2f\n", num1*num2);
            break;
            case '/':
            if (num2 == 0)
            {
                printf("Cannot divide by zero\n");
            }
            else
            {
                printf("Result: %.2f\n", num1 / num2);
            }
            break;
            default:
            printf("Unknown operator\n");
        }
        printf("Again? (y/n): ");
        scanf(" %c", &repeat);
    } while (repeat == 'y'||repeat == 'Y');
    printf("\n--- Results ---\n");
    return 0;
}
