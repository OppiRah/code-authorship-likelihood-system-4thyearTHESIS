/*
 * Name: Vanessa Valdez
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
        if (operation == '/' && num2 == 0)
        {
            printf("Error: division by zero\n");
        }
        else
        {
            switch (operation)
            {
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
        printf("Again? (y/n): ");
        scanf(" %c", &repeat);
    } while (repeat == 'y' || repeat == 'Y');
    return 0;
}
