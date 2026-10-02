/*
 * Name: Sean Salazar
 * Program: Linear Search
 */

#include <stdio.h>

int linear_search(int numbers[], int size, int key) {
    for (int i = 0; i < size; ++i) {
        if (numbers[i] == key) {
            return i;
        }
    }
    return -1;  // not found
}

int main() {
    int numbers[] = {5, 17, 3, 42, 8, 29};
    int key;
    printf("Search for: ");
    scanf("%d", &key);
    int index = linear_search(numbers, 6, key);
    if (index >= 0) {
        printf("Found at %d\n", index);
    } else {
        puts("Not found");
    }
    return 0;
}
