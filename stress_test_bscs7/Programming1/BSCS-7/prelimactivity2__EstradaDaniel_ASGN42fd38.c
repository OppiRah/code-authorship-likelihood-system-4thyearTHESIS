/*
 * Name: Daniel Estrada
 * Program: Temperature Converter
 */

#include <stdio.h>

int main(void)
{
    float celsius, fahrenheit;
    int choice;
    printf("1 - Celsius to Fahrenheit\n2 - Fahrenheit to Celsius\n");
    scanf("%d", &choice);

    if (choice == 1)
    {
        scanf("%f", &celsius);
        // C to F
        fahrenheit = celsius * 9 / 5 + 32;
        printf("%.2f F\n", fahrenheit);
    }
    else if (choice==2)
    {
        scanf("%f", &fahrenheit);
        // F to C
        celsius = (fahrenheit - 32) * 5 / 9;
        printf("%.2f C\n", celsius);
    }
    else
    {
        puts("Invalid choice");
    }
    return 0;
}
