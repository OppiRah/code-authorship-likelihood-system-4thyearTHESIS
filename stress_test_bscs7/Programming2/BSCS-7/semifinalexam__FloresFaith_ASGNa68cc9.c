// Faith Flores - Sort, Search and Median

#include <stdio.h>

void sortArray(int numbers[], int size) {
    int swapped;
    for (int i = 0; i < size - 1; i += 1) {
        swapped = 0;
        for (int j = 0; j < size - i - 1; j += 1) {
            if (numbers[j] > numbers[j + 1]) {
                int temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1]=temp;
                swapped = 1;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

void printArray(int numbers[], int size) {
    for (int i = 0; i < size; i += 1) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
}

int binarySearch(int numbers[], int size, int key) {
    // array is sorted so we can stop early
    for (int i = 0; i < size && numbers[i] <= key; i+=1) {
        if (numbers[i] == key) {
            return i;
        }
    }
    return -1;
}

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    if (size <= 0 || size > 50) {
        printf("Invalid size\n");
        return 1;
    }
    for (int i=0; i < size; i += 1) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    sortArray(numbers, size);
    printf("Sorted: ");
    printArray(numbers, size);
    // median of the sorted list
    float median;
    if (size % 2 == 0) {
        median = (numbers[size / 2 - 1] + numbers[size / 2]) / 2.0;
    } else {
        median = numbers[size / 2];
    }
    printf("Median: %.1f\n", median);
    int key;
    printf("Search for: ");
    scanf("%d", &key);
    int index = binarySearch(numbers, size, key);
    printf(index >= 0 ? "Found at %d\n" : "Not found\n", index);

    printf("Descending: ");
    for (int i = size - 1; i >= 0; i--) {
        printf("%d ", numbers[i]);
    }
    printf("\n");

    int total = 0;
    for (int i = 0; i < size; i += 1) {
        total = total + numbers[i];
    }
    printf("Sum: %d, Average: %.2f\n", total, total * 1.0/size);

    printf("Smallest: %d, Largest: %d\n", numbers[0], numbers[size - 1]);
    return 0;
}
