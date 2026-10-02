/*
 * Name: Francis Galang
 * Program: Simple Calculator
 */

#include <stdio.h>

int main(void)
{
    double num1, num2, result;
    char operation;

    printf("Enter first number: ");
    scanf("%lf", &num1);
    printf("Enter operator (+ - * /): ");
    scanf(" %c", &operation);
    printf("Enter second number: ");
    scanf("%lf", &num2);
    if (operation == '+')
    {
        result = num1 + num2;
        printf("Result: %.2f\n", result);
    }
    else if (operation == '-')
    {
        result = num1 - num2;
        printf("Result: %.2f\n", result);
    }
    else if (operation == '*')
    {
        result = num1 * num2;
        printf("Result: %.2f\n", result);
    }
    else if (operation == '/')
    {
        if (num2 != 0)
        {
            result = num1 / num2;
            printf("Result: %.2f\n", result);
        }
        else
        {
            printf("Cannot divide by zero\n");
        }
    }
    else
    {
        printf("Unknown operator\n");
    }
    printf("\n--- Results ---\n");
}
