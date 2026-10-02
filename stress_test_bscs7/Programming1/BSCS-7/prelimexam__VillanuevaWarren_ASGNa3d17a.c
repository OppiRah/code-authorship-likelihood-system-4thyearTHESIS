/*
 * Name: Warren Villanueva
 * Program: Grade Computation
 */

#include <stdio.h>

int main() {
    float grades[4], total = 0, average;
    char letter;
    for (int i = 0; i < 4; i += 1) {  // input
        printf("Grade %d: ", i + 1);
        scanf("%f", &grades[i]);
        total = total + grades[i];
    }
    average = total/4;  // compute the average
    if (average < 0||average > 100) {
        printf("Invalid grades\n");
        return 1;
    }

    if (average >= 90) {  // letter grade
        letter='A';
        printf("Remarks: Passed\n");
    } else if (average >= 80) {
        letter='B';
        printf("Remarks: Passed\n");
    } else if (average>=75) {
        letter = 'C';
        printf("Remarks: Passed\n");
    } else {
        letter = 'F';
        printf("Remarks: Failed\n");
    }

    printf("Average: %.2f\n", average);
    printf("Grade: %c\n", letter);
}
