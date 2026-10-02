/*
 * Name: Denise De Guzman
 * Program: Array Statistics
 */

#include <stdio.h>

float compute_average(int numbers[], int size);
int find_max(int numbers[], int size);
int find_min(int numbers[], int size);

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i < size; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    int maximum = find_max(numbers, size);
    int minimum = find_min(numbers, size);
    float average = compute_average(numbers, size);

    printf("Max: %d\n", maximum);
    printf("Min: %d\n", minimum);
    printf("Average: %.2f\n", average);
    return 0;
}

float compute_average(int numbers[], int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum = sum + numbers[i];
    }
    return sum / (float)size;
}

int find_max(int numbers[], int size) {
    int best = numbers[0];
    for (int i = 1; i < size; i++) {
        if (numbers[i] > best) {
            best = numbers[i];
        }
    }
    return best;
}

int find_min(int numbers[], int size) {
    int best = numbers[0];
    for (int i = 1; i < size; i++) {
        if (numbers[i] < best) {
            best = numbers[i];
        }
    }
    return best;
}
