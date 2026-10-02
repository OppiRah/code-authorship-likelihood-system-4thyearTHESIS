/*
 * Name: Mikaela Navarro
 * Program: Array Sum and Average
 */

#include <stdio.h>

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i<size; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    int sum=0;

    for (int i = 0; i < size; i++) {  // add all elements
        sum = sum + numbers[i];
    }
    printf("Sum = %d\n", sum);
    printf("Average = %.2f\n", (float)sum / size);
    return 0;
}
