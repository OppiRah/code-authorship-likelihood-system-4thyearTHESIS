// Carlo Cruz - Grade Computation

#include <stdio.h>

int main() {
    float grade1, grade2, grade3, average;
    char letter;

    printf("Enter grade: ");
    scanf("%f", &grade1);
    printf("Enter grade: ");
    scanf("%f", &grade2);
    printf("Enter grade: ");
    scanf("%f", &grade3);
    average = (grade1 + grade2 + grade3) / 3;  // compute the average

    if (average >= 90) {
        letter = 'A';
    } else if (average >= 80) {
        letter = 'B';
    } else if (average >= 75) {
        letter = 'C';
    } else {
        letter = 'F';
    }

    printf("\n--- Results ---\n");
    printf("Average: %.2f\n", average);
    printf("Grade: %c\n", letter);
    printf("Remarks: %s\n", average >= 75 ? "Passed" : "Failed");
    return 0;
}
