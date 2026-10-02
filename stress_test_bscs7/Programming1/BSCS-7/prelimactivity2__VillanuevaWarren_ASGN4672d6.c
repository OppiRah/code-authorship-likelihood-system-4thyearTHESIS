/*
 * Name: Warren Villanueva
 * Program: Temperature Converter
 */

#include <stdio.h>

int main() {
    float celsius, fahrenheit;
    int choice;

    printf("1 - Celsius to Fahrenheit\n2 - Fahrenheit to Celsius\n");
    printf("Choice: ");
    scanf("%d", &choice);

    if (choice==1) {
        printf("Enter Celsius: ");
        scanf("%f", &celsius);
        fahrenheit = celsius * 9 / 5 + 32;  // C to F
        printf("%.2f F\n", fahrenheit);
    } else if (choice == 2) {
        printf("Enter Fahrenheit: ");
        scanf("%f", &fahrenheit);
        celsius = (fahrenheit - 32) * 5 / 9;
        printf("%.2f C\n", celsius);
    } else {
        printf("Invalid choice\n");
    }
}
