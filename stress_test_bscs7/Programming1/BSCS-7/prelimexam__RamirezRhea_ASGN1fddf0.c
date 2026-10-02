/*
 * Name: Rhea Ramirez
 * Program: Grade Computation
 */

#include <stdio.h>

int main(void)
{
    float grades[3], total = 0, average;
    char letter;

    for (int i = 0; i < 3; i += 1)
    {
        printf("Grade %d: ", i + 1);
        scanf("%f", &grades[i]);
        total = total + grades[i];
    }
    // compute the average
    average = total / 3;

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

    printf("Average: %.2f\nGrade: %c\nRemarks: %s\n", average, letter, average >= 75 ? "Passed" : "Failed");
}
