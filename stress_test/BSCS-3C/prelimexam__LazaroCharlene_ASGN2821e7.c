// Programming exercise: Grade Average and Letter Grade
// Submitted by: Charlene Lazaro

#include <stdio.h>

int main()
{
    // compute average of 5 scores and assign a letter grade
    int n = 5;
    int sco[5] = { 88, 92, 76, 81, 95 };
    int sum = 0;
    for (int i = 0; i<n; i++) {
        sum = sum+sco[i];
    }
    float avg = (float)sum / n;
    char grade;
    if (avg >= 90) grade = 'A';
    else if (avg >= 80) grade = 'B';
    else if (avg >= 70) grade = 'C';
    else grade = 'F';
    printf("Average: %.2f, Grade: %c\n", avg, grade);
    return 0;
}
