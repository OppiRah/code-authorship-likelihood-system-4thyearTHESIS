// Carlo Cruz - Array Statistics

#include <stdio.h>

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    if (size <= 0 || size > 50) {
        puts("Invalid size");
        return 1;
    }
    for (int i = 0; i < size; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    printf("You entered: ");
    for (int i=0; i < size; i++) {
        printf("%d ", numbers[i]);
    }
    puts("");
    int maximum = numbers[0], minimum = numbers[0], sum = 0;
    int i;
    // one pass for everything
    i = 0;
    while (i < size) {
        sum += numbers[i];
        if (numbers[i] > maximum) {
            maximum = numbers[i];
        }
        if (numbers[i] < minimum) {
            minimum = numbers[i];
        }
        i++;
    }
    float average = (float)sum / size;
    int above = 0;
    for (int i = 0; i < size; i++) {
        if (numbers[i] > average) {
            above++;
        }
    }

    printf("Max: %d\n", maximum);
    printf("Min: %d\n", minimum);
    printf("Average: %.2f\n", average);
    printf("Above average: %d\n", above);
    return 0;
}
