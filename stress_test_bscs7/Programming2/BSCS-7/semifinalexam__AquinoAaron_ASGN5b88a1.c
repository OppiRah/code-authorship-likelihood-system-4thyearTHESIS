// Aaron Aquino - Sort, Search and Median

#include <stdio.h>

int binarySearch(int numbers[], int size, int key) {
    int low = 0, high = size - 1;
    while (low <= high) {
        int mid = (low + high) / 2;  // middle index
        if (numbers[mid] == key) {
            return mid;
        } else if (numbers[mid] < key) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

void sortArray(int numbers[], int size) {
    for (int i = 0; i < size - 1; ++i) {
        for (int j = 0; j < size - i - 1; ++j) {
            if (numbers[j] > numbers[j + 1]) {
                int temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
            }
        }
    }
}

void printArray(int numbers[], int size) {
    for (int i = 0; i < size; ++i) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
}

int main() {
    int numbers[50], size;
    scanf("%d", &size);
    if (size <= 0 || size > 50) {
        printf("Invalid size\n");
        return 1;
    }
    for (int i = 0; i < size; ++i) {
        scanf("%d", &numbers[i]);
    }
    sortArray(numbers, size);
    printf("Sorted: ");
    printArray(numbers, size);
    float median=(size % 2) ? numbers[size / 2] : (numbers[size / 2 - 1] + numbers[size / 2]) / 2.0;
    printf("Median: %.1f\n", median);
    int key;
    scanf("%d", &key);
    int index = binarySearch(numbers, size, key);
    printf(index >= 0 ? "Found at %d\n" : "Not found\n", index);
    int duplicates = 0;
    for (int i = 1; i < size; ++i) {
        if (numbers[i] == numbers[i - 1]) {
            ++duplicates;
        }
    }
    printf("Duplicates: %d\n", duplicates);
    return 0;
}
