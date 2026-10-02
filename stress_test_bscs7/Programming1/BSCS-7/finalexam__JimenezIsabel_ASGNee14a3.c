/*
 * Name: Isabel Jimenez
 * Program: Array Statistics
 */

#include <stdio.h>

int main(void) {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i < size; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }

    int maximum = numbers[0], minimum = numbers[0], sum = 0;
    for (int i = 0; i < size; i++) {
        if (numbers[i] > maximum) {
            maximum = numbers[i];
        } else if (numbers[i] < minimum) {
            minimum = numbers[i];
        }
    }
    for (int i = 0; i < size; i++) {  // total
        sum = sum + numbers[i];
    }
    float average = (float)sum / size;
    int above = 0;
    for (int i = 0; i < size; i++) {
        if (numbers[i] > average) {
            above++;
        }
    }

    printf("Max: %d\nMin: %d\nAverage: %.2f\nAbove average: %d\n", maximum, minimum, average, above);
    return 0;
}
