/*
 * Name: Quincy Quiambao
 * Program: Grade Computation
 */

#include <stdio.h>

int main(void)
{
    float grades[3], total = 0, average;
    char letter;
    for (int i = 0; i < 3; i += 1)
    {
        printf("Grade %d: ", i+1);
        scanf("%f", &grades[i]);
        total += grades[i];
    }
    average = total / 3;
    if (average < 0 || average > 100)
    {
        puts("Invalid grades");
        return 1;
    }

    // letter grade
    switch ((int)average / 10)
    {
        case 10:
        case 9: letter = 'A'; break;
        case 8: letter = 'B'; break;
        case 7: letter = (average >= 75) ? 'C' : 'F'; break;
        default: letter = 'F';
    }

    printf("Average: %.2f\n", average);
    printf("Grade: %c\n", letter);
    if (average >= 75)
    {
        puts("Remarks: Passed");
    }
    else
    {
        puts("Remarks: Failed");
    }
    printf("\n--- Results ---\n");
    return 0;
}
