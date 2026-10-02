// Faith Flores - Grade Computation

#include <stdio.h>

int main() {
    float value, total = 0, average;
    char letter;
    // input
    int i = 0;
    while (i < 4) {
        printf("Enter grade %d: ", i + 1);
        scanf("%f", &value);
        total = total + value;
        i += 1;
    }
    average = total / 4;
    if (average<0 || average > 100) {
        printf("Invalid grades\n");
        return 1;
    }

    if (average < 75) {
        letter = 'F';
    } else if (average < 80) {
        letter = 'C';
    } else if (average < 90) {
        letter = 'B';
    } else {
        letter = 'A';
    }
    printf("Average: %.2f\n", average);
    printf("Grade: %c\n", letter);
    if (average >= 75) {
        printf("Remarks: Passed\n");
    } else {
        printf("Remarks: Failed\n");
    }
    return 0;
}
