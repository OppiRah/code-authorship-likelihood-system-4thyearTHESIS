// Erika Fajardo - Grade Computation

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
        ++i;
    }
    average=total / 4;

    switch ((int)average / 10) {
        case 10:
        case 9: letter='A'; break;
        case 8: letter = 'B'; break;
        case 7: letter = (average >= 75) ? 'C' : 'F'; break;
        default: letter = 'F';
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
