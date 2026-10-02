/*
 * Name: Warren Villanueva
 * Program: Sort, Search and Median
 */

#include <stdio.h>

int binary_search(int numbers[], int size, int key) {
    for (int i = 0; i < size && numbers[i] <= key; i += 1) {  // array is sorted so we can stop early
        if (numbers[i] == key) {
            return i;
        }
    }
    return -1;
}

void sort_array(int numbers[], int size) {
    for (int i = 0; i < size - 1; i += 1) {
        for (int j = 0; j < size - i - 1; j += 1) {
            if (numbers[j] > numbers[j + 1]) {
                int temp = numbers[j];
                numbers[j]=numbers[j + 1];
                numbers[j + 1] = temp;
            }
        }
    }
}

void print_array(int numbers[], int size) {
    for (int i = 0; i<size; i += 1) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
}

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    if (size <= 0 || size > 50) {
        printf("Invalid size\n");
        return 1;
    }
    for (int i = 0; i < size; i+=1) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    sort_array(numbers, size);
    printf("Sorted: ");
    print_array(numbers, size);
    float median = (size%2) ? numbers[size / 2] : (numbers[size / 2-1] + numbers[size / 2]) / 2.0;
    printf("Median: %.1f\n", median);
    int key;
    printf("Search for: ");
    scanf("%d", &key);
    int index = binary_search(numbers, size, key);
    printf(index >= 0 ? "Found at %d\n" : "Not found\n", index);
    printf("Descending: ");
    for (int i = size - 1; i >= 0; i--) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
}
