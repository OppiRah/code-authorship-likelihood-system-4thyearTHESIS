// Patricia Pangilinan - Array Sum and Average

#include <stdio.h>

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i < size; ++i) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    int sum = 0;

    // add all elements
    for (int i = 0; i < size; ++i) {
        sum += numbers[i];
    }
    printf("Sum = %d\n", sum);
    printf("Average = %.2f\n", (float)sum / size);
    return 0;
}
