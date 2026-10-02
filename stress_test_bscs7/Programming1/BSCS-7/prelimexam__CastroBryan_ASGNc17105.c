/*
 * Name: Bryan Castro
 * Program: Grade Computation
 */

#include <stdio.h>

int main(void)
{
    float grade1, grade2, grade3, grade4, average;
    char letter;

    // input
    printf("Enter grade: ");
    scanf("%f", &grade1);
    printf("Enter grade: ");
    scanf("%f", &grade2);
    printf("Enter grade: ");
    scanf("%f", &grade3);
    printf("Enter grade: ");
    scanf("%f", &grade4);
    // compute the average
    average = (grade1 + grade2 + grade3 + grade4) / 4;
    if (average < 0 || average > 100)
    {
        printf("Invalid grades\n");
        return 1;
    }

    // letter grade
    if (average < 75)
    {
        letter = 'F';
    }
    else if (average < 80)
    {
        letter = 'C';
    }
    else if (average < 90)
    {
        letter = 'B';
    }
    else
    {
        letter = 'A';
    }

    printf("Average: %.2f\n", average);
    printf("Grade: %c\n", letter);
    if (average >= 75)
    {
        printf("Remarks: Passed\n");
    }
    else
    {
        printf("Remarks: Failed\n");
    }
    return 0;
}
