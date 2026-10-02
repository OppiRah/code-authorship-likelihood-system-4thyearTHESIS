// Carlo Cruz - Bubble Sort

#include <stdio.h>

void printArray(int numbers[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", numbers[i]);
    }
    puts("");
}

void sortArray(int numbers[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (numbers[j] > numbers[j + 1]) {
                int temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
            }
        }
    }
}

int main() {
    int numbers[] = {64, 25, 12, 22, 11, 90, 38};
    int size = 7;
    printf("Before: ");
    printArray(numbers, size);
    sortArray(numbers, size);
    printf("After: ");
    printArray(numbers, size);
    return 0;
}
