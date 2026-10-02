/*
 * Name: Warren Villanueva
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
    if (size <= 0 || size > 50) {
        printf("Invalid size\n");
        return 1;
    }
    for (int i = 0; i < size; i += 1) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }

    int maximum = find_max(numbers, size);
    int minimum = find_min(numbers, size);
    float average = compute_average(numbers, size);
    int above = 0;
    for (int i=0; i < size; i += 1) {
        if (numbers[i] > average) {
            above += 1;
        }
    }
    printf("Max: %d\nMin: %d\nAverage: %.2f\nAbove average: %d\n", maximum, minimum, average, above);
}

float compute_average(int numbers[], int size) {
    int sum=0;
    for (int i = 0; i<size; i += 1) {
        sum = sum + numbers[i];
    }
    return sum / (float)size;
}

int find_max(int numbers[], int size) {
    int best = numbers[0];
    for (int i = 1; i < size; i += 1) {
        if (numbers[i]>best) {
            best = numbers[i];
        }
    }
    return best;
}

int find_min(int numbers[], int size) {
    int best = numbers[0];
    for (int i = 1; i < size; i+=1) {
        if (numbers[i] < best) {
            best = numbers[i];
        }
    }
    return best;
}
