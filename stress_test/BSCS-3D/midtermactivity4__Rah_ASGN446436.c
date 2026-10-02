#include <stdio.h>

int main() {
    // Activity 4: Bubble sort implementation

    int dataArray[20];
    int arraySize;

    printf("Enter how many numbers to sort: ");
    scanf("%d", &arraySize);

    for (int i = 0; i < arraySize; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &dataArray[i]);
    }

    // Bubble sort: repeatedly swap adjacent elements if out of order
    for (int pass = 0; pass < arraySize - 1; pass++) {
        for (int j = 0; j < arraySize - pass - 1; j++) {
            if (dataArray[j] > dataArray[j + 1]) {
                int temp = dataArray[j];
                dataArray[j] = dataArray[j + 1];
                dataArray[j + 1] = temp;
            }
        }
    }

    printf("\nSorted array: ");
    for (int i = 0; i < arraySize; i++) {
        printf("%d ", dataArray[i]);
    }
    printf("\n");

    return 0;
}
