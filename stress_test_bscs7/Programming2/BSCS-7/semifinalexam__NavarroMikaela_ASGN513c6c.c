/*
 * Name: Mikaela Navarro
 * Program: Sort, Search and Median
 */

#include <stdio.h>

void sort_array(int numbers[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j=0; j < size - i - 1; j++) {
            if (numbers[j] > numbers[j + 1]) {
                int temp=numbers[j];
                numbers[j]=numbers[j+1];
                numbers[j + 1] = temp;
            }
        }
    }
}

void print_array(int numbers[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
}

int binary_search(int numbers[], int size, int key) {
    for (int i = 0; i < size && numbers[i] <= key; i++) {  // array is sorted so we can stop early
        if (numbers[i]==key) {
            return i;
        }
    }
    return -1;
}

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i=0; i < size; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    sort_array(numbers, size);
    printf("Sorted: ");
    print_array(numbers, size);
    float median;  // median of the sorted list
    if (size % 2 == 0) {
        median = (numbers[size / 2 - 1] + numbers[size / 2]) / 2.0;
    } else {
        median = numbers[size / 2];
    }
    printf("Median: %.1f\n", median);
    int key;
    printf("Search for: ");
    scanf("%d", &key);
    int index = binary_search(numbers, size, key);
    printf(index >= 0 ? "Found at %d\n" : "Not found\n", index);

    printf("Smallest: %d, Largest: %d\n", numbers[0], numbers[size - 1]);
    printf("Descending: ");
    for (int i = size - 1; i >= 0; i--) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    return 0;
}
