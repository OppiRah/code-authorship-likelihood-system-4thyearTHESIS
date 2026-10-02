// Patricia Pangilinan - Array Statistics

#include <stdio.h>

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    if (size <= 0 || size > 50) {
        puts("Invalid size");
        return 1;
    }
    for (int i = 0; i < size; ++i) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }

    int maximum = numbers[0], minimum = numbers[0], sum = 0;
    int i;
    i=0;
    while (i < size) {
        if (numbers[i] > maximum) {
            maximum = numbers[i];
        } else if (numbers[i] < minimum) {
            minimum = numbers[i];
        }
        ++i;
    }
    i = 0;
    while (i < size) {
        sum = sum + numbers[i];
        ++i;
    }
    float average = (float)sum / size;
    int above = 0;
    for (int i = 0; i < size; ++i) {
        if (numbers[i] > average) {
            ++above;
        }
    }
    printf("\n--- Results ---\n");
    printf("Max: %d\nMin: %d\nAverage: %.2f\nAbove average: %d\n", maximum, minimum, average, above);
    return 0;
}
