#include <stdio.h>

char getGrade(float avg) {
    if (avg >= 90) return 'A';
    if (avg >= 80) return 'B';
    if (avg >= 70) return 'C';
    return 'F';
}

int main() {
    // compute average of 5 scores using a helper function
    int scores[5] = { 88, 92, 76, 81, 95 };
    int total = 0;
    for (int i = 0; i<5; i++) total += scores[i];
    float average = (float)total / 5;
    char letter = getGrade(average);
    printf("Average: %.2f, Grade: %c\n", average, letter);
    return 0;
}
