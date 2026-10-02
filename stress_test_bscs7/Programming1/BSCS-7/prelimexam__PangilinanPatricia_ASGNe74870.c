// Patricia Pangilinan - Grade Computation

#include <stdio.h>

int main() {
    float value, total = 0, average;
    char letter;
    // input
    int i = 0;
    while (i < 3) {
        printf("Enter grade %d: ", i + 1);
        scanf("%f", &value);
        total = total + value;
        ++i;
    }
    average = total / 3;
    if (average >= 90) {
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
    if (average >= 75) {
        puts("Remarks: Passed");
    } else {
        puts("Remarks: Failed");
    }
    return 0;
}
