#include <stdio.h>

int main() {
    // Activity 1: Multiplication table using a for loop

    int tableNumber;

    printf("Enter a number to generate its multiplication table: ");
    scanf("%d", &tableNumber);

    printf("\nMultiplication table for %d:\n", tableNumber);

    for (int multiplier = 1; multiplier <= 10; multiplier++) {
        printf("%d x %d = %d\n", tableNumber, multiplier, tableNumber * multiplier);
    }

    return 0;
}
