/*
 * Name: Denise De Guzman
 * Program: Array Maximum
 */

#include <stdio.h>

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i < size; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    int maximum = numbers[0], position = 0;
    for (int i = 1; i < size; i++) {  // look for the largest element
        if (numbers[i] > maximum) {
            maximum = numbers[i];
            position = i;  // remember where
        }
    }
    printf("Max = %d at index %d\n", maximum, position);
    return 0;
}
