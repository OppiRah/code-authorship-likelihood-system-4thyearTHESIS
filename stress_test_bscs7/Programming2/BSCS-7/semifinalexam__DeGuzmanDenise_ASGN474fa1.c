/*
 * Name: Denise De Guzman
 * Program: Sort, Search and Median
 */

#include <stdio.h>

void sort_array(int numbers[], int size);
void print_array(int numbers[], int size);
int binary_search(int numbers[], int size, int key);

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    if (size <= 0 || size > 50) {
        printf("Invalid size\n");
        return 1;
    }
    for (int i = 0; i < size; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    printf("You entered: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    sort_array(numbers, size);
    printf("Sorted: ");
    print_array(numbers, size);
    float median = (size % 2) ? numbers[size / 2] : (numbers[size / 2 - 1] + numbers[size / 2]) / 2.0;  // median of the sorted list
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
    printf("\n--- Results ---\n");
    printf("\n");
    return 0;
}

void sort_array(int numbers[], int size) {
    for (int i = 0; i < size-1; i++) {
        int min_index = i;
        for (int j = i + 1; j < size; j++) {
            if (numbers[j] < numbers[min_index]) {
                min_index=j;
            }
        }
        int temp = numbers[i];
        numbers[i] = numbers[min_index];
        numbers[min_index] = temp;
    }
}

void print_array(int numbers[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
}

int binary_search(int numbers[], int size, int key) {
    int low = 0, high = size - 1;
    while (low <= high) {
        int mid = (low + high) / 2;  // middle index
        if (numbers[mid] == key) {
            return mid;
        } else if (numbers[mid] < key) {
            low = mid + 1;
        } else {
            high=mid-1;
        }
    }
    return -1;
}
