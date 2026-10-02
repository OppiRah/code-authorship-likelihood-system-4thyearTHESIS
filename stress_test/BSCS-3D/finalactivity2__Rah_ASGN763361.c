#include <stdio.h>

// Activity 2: Swap two numbers using pointers

void swapValues(int *pointerA, int *pointerB) {
    int temporary = *pointerA;
    *pointerA = *pointerB;
    *pointerB = temporary;
}

int main() {
    int firstValue, secondValue;

    printf("Enter the first value: ");
    scanf("%d", &firstValue);

    printf("Enter the second value: ");
    scanf("%d", &secondValue);

    printf("\nBefore swap: %d, %d\n", firstValue, secondValue);

    swapValues(&firstValue, &secondValue);

    printf("After swap: %d, %d\n", firstValue, secondValue);

    return 0;
}
