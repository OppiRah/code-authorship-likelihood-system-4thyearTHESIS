/*
 * Name: Quincy Quiambao
 * Program: Average of Three Grades
 */

#include <stdio.h>

int main(void)
{
    float grade1, grade2, grade3, average;

    // read the three grades
    printf("Enter three grades: ");
    scanf("%f %f %f", &grade1, &grade2, &grade3);
    // average of three
    average = (grade1 + grade2 + grade3) / 3;
    printf("Average: %.2f\n", average);
    return 0;
}
