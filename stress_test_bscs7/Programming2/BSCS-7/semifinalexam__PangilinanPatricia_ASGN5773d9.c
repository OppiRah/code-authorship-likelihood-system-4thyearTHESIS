// Patricia Pangilinan - Sort, Search and Median

#include <stdio.h>

int binarySearch(int numbers[], int size, int key) {
    for (int i = 0; i < size && numbers[i] <= key; ++i) {
        if (numbers[i] == key) {
            return i;
        }
    }
    return -1;
}

void printArray(int numbers[], int size) {
    for (int i = 0; i<size; ++i) {
        printf("%d ", numbers[i]);
    }
    puts("");
}

void sortArray(int numbers[], int size) {
    int swapped;
    for (int i = 0; i < size - 1; ++i) {
        swapped = 0;
        for (int j = 0; j < size - i - 1; ++j) {
            if (numbers[j] > numbers[j + 1]) {
                int temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
                swapped = 1;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i < size; ++i) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    sortArray(numbers, size);
    printf("Sorted: ");
    printArray(numbers, size);
    // median of the sorted list
    float median = (size % 2) ? numbers[size / 2] : (numbers[size / 2 - 1] + numbers[size / 2]) / 2.0;
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
    puts("");

    int duplicates = 0;
    for (int i = 1; i < size; ++i) {
        if (numbers[i] == numbers[i - 1]) {
            ++duplicates;
        }
    }
    printf("Duplicates: %d\n", duplicates);
    return 0;
}
