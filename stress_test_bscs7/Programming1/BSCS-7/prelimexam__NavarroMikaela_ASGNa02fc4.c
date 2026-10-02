/*
 * Name: Mikaela Navarro
 * Program: Grade Computation
 */

#include <stdio.h>

int main() {
    float grades[4], total = 0, average;
    char letter;
    for (int i = 0; i < 4; i++) {  // input
        printf("Grade %d: ", i + 1);
        scanf("%f", &grades[i]);
        total = total + grades[i];
    }
    average = total / 4;
    if (average < 0 || average > 100) {
        printf("Invalid grades\n");
        return 1;
    }
    if (average < 75) {
        letter='F';
    } else if (average < 80) {
        letter = 'C';
    } else if (average < 90) {
        letter = 'B';
    } else {
        letter = 'A';
    }
    printf("Average: %.2f\n", average);
    printf("Grade: %c\n", letter);
    if (average >= 75) {
        printf("Remarks: Passed\n");
    } else {
        printf("Remarks: Failed\n");
    }
    return 0;
}
