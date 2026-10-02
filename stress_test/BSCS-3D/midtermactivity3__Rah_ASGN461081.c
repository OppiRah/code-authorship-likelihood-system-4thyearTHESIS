#include <stdio.h>

int main() {
    // Activity 3: Find the maximum and minimum values in an array

    int numberArray[20];
    int arraySize;

    printf("Enter how many numbers to input: ");
    scanf("%d", &arraySize);

    for (int i = 0; i < arraySize; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numberArray[i]);
    }

    int maxValue = numberArray[0];
    int minValue = numberArray[0];

    // Scan through the array to find max and min
    for (int i = 1; i < arraySize; i++) {
        if (numberArray[i] > maxValue) {
            maxValue = numberArray[i];
        }
        if (numberArray[i] < minValue) {
            minValue = numberArray[i];
        }
    }

    printf("\nMaximum value: %d\n", maxValue);
    printf("Minimum value: %d\n", minValue);

    return 0;
}
