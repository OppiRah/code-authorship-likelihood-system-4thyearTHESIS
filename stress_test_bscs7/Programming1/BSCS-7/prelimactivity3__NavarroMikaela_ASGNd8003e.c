/*
 * Name: Mikaela Navarro
 * Program: Average of Three Grades
 */

#include <stdio.h>

int main() {
    float grade1, grade2, grade3, average;
    printf("Enter three grades: ");  // read the three grades
    scanf("%f %f %f", &grade1, &grade2, &grade3);
    average = (grade1 + grade2 + grade3)/3;  // average of three
    printf("Average: %.2f\n", average);
    if (average >= 75) {
        printf("Passed\n");
    } else {
        printf("Failed\n");
    }
    return 0;
}
