/*
 * Name: Bryan Castro
 * Program: Temperature Converter
 */

#include <stdio.h>

int main(void)
{
    float celsius, fahrenheit;
    int choice;
    printf("1 - Celsius to Fahrenheit\n2 - Fahrenheit to Celsius\n");
    printf("Choice: ");
    scanf("%d", &choice);

    if (choice == 1)
    {
        printf("Enter Celsius: ");
        scanf("%f", &celsius);
        // C to F
        fahrenheit = celsius * 9 / 5 + 32;
        printf("%.2f F\n", fahrenheit);
    }
    else if (choice == 2)
    {
        printf("Enter Fahrenheit: ");
        scanf("%f", &fahrenheit);
        // F to C
        celsius = (fahrenheit - 32) * 5 / 9;
        printf("%.2f C\n", celsius);
    }
    else
    {
        printf("Invalid choice\n");
    }
    return 0;
}
