// Aaron Aquino - Grade Computation

#include <stdio.h>

int main() {
    float grades[3], total = 0, average;
    char letter;

    for (int i = 0; i < 3; ++i) {
        scanf("%f", &grades[i]);
        total = total + grades[i];
    }
    average = total / 3;  // compute the average
    switch ((int)average / 10) {
        case 10:
        case 9: letter = 'A'; break;
        case 8: letter = 'B'; break;
        case 7: letter = (average >= 75) ? 'C' : 'F'; break;
        default: letter = 'F';
    }

    printf("Average: %.2f\n", average);
    if (average >= 75) {
        printf("Remarks: Passed\n");
    } else {
        printf("Remarks: Failed\n");
    }
    printf("\n--- Results ---\n");
    return 0;
}
