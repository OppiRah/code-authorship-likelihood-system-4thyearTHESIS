/*
 * Name: Quincy Quiambao
 * Program: Simple Calculator
 */

#include <stdio.h>

int main(void)
{
    double num1, num2;
    char operation;

    printf("Enter first number: ");
    scanf("%lf", &num1);
    printf("Enter operator (+ - * /): ");
    scanf(" %c", &operation);
    printf("Enter second number: ");
    scanf("%lf", &num2);
    if (operation == '/' && num2==0)
    {
        puts("Error: division by zero");
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
            puts("Unknown operator");
        }
    }
    return 0;
}
