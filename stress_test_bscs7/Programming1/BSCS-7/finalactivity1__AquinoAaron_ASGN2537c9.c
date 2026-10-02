// Aaron Aquino - Array Sum and Average

#include <stdio.h>

int main() {
    int numbers[50], size;
    scanf("%d", &size);
    for (int i = 0; i < size; ++i) {
        scanf("%d", &numbers[i]);
    }
    int sum = 0;

    // add all elements
    for (int i = 0; i < size; ++i) {
        sum = sum + numbers[i];
    }
    printf("Sum = %d\n", sum);
    printf("Average = %.2f\n", sum * 1.0 / size);
    return 0;
}
