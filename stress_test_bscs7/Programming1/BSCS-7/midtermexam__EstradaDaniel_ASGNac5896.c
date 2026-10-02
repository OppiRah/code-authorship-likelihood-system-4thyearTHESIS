/*
 * Name: Daniel Estrada
 * Program: Simple Calculator
 */

#include <stdio.h>

int main(void)
{
    float num1, num2;
    char operation;
    char repeat = 'y';

    while (repeat == 'y' || repeat == 'Y')
    {
        // read the expression
        scanf("%f", &num1);
        scanf(" %c", &operation);
        scanf("%f", &num2);
        if (operation == '/' && num2 == 0)
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
                printf("Result: %.2f\n", num1-num2);
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
        scanf(" %c", &repeat);
    }
    return 0;
}
