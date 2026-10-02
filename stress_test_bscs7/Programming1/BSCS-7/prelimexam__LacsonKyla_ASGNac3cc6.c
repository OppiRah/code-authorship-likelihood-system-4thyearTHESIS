/*
 * Name: Kyla Lacson
 * Program: Grade Computation
 */

#include <stdio.h>

int main(void)
{
    float grades[4], total = 0, average;
    char letter;

    for (int i = 0; i < 4; i++)  // input
    {
        printf("Grade %d: ", i + 1);
        scanf("%f", &grades[i]);
        total += grades[i];
    }
    average=total / 4;  // compute the average

    if (average >= 90)
    {
        letter = 'A';
    }
    else if (average >= 80)
    {
        letter='B';
    }
    else if (average >= 75)
    {
        letter = 'C';
    }
    else
    {
        letter = 'F';
    }

    printf("\n--- Results ---\n");
    printf("Average: %.2f\n", average);
    printf("Grade: %c\n", letter);
    printf("Remarks: %s\n", average >= 75 ? "Passed" : "Failed");
    return 0;
}
