// Aaron Aquino - Temperature Converter

#include <stdio.h>

int main() {
    float celsius, fahrenheit;
    int choice;

    printf("1 - Celsius to Fahrenheit\n2 - Fahrenheit to Celsius\n");
    scanf("%d", &choice);
    if (choice == 1) {
        scanf("%f", &celsius);
        fahrenheit = celsius * 9 / 5+32;
        printf("%.2f F\n", fahrenheit);
    } else if (choice == 2) {
        scanf("%f", &fahrenheit);
        celsius = (fahrenheit - 32) * 5 / 9;  // F to C
        printf("%.2f C\n", celsius);
    } else {
        printf("Invalid choice\n");
    }
    return 0;
}
