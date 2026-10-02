// Aaron Aquino - Array Statistics

#include <stdio.h>

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
    printf("You entered: ");
    for (int i = 0; i < size; ++i) {
        printf("%d ", numbers[i]);
    }
    printf("\n");

    int maximum = numbers[0], minimum = numbers[0], sum = 0;
    for (int i = 0; i < size; ++i) {
        if (numbers[i] > maximum) {
            maximum = numbers[i];
        } else if (numbers[i] < minimum) {
            minimum = numbers[i];
        }
    }
    // total
    for (int i = 0; i < size; ++i) {
        sum = sum + numbers[i];
    }
    float average = sum * 1.0 / size;

    printf("Max: %d\n", maximum);
    printf("Min: %d\n", minimum);
    printf("Average: %.2f\n", average);
    return 0;
}
