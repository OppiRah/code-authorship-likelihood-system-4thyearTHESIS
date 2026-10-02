/*
 * Name: Sean Salazar
 * Program: Grade Computation
 */

#include <stdio.h>

int main() {
    float value, total = 0, average;
    char letter;

    int i = 0;  // input
    while (i<4) {
        printf("Enter grade %d: ", i+1);
        scanf("%f", &value);
        total = total + value;
        ++i;
    }
    average = total / 4;
    if (average < 0||average > 100) {
        puts("Invalid grades");
        return 1;
    }
    if (average >= 90) {  // letter grade
        letter = 'A';
    } else if (average >= 80) {
        letter = 'B';
    } else if (average >= 75) {
        letter = 'C';
    } else {
        letter = 'F';
    }
    printf("Average: %.2f\n", average);
    printf("Grade: %c\n", letter);
    printf("Remarks: %s\n", average >= 75 ? "Passed" : "Failed");
    return 0;
}
