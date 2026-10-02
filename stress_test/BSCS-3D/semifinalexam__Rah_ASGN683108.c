#include <stdio.h>

// Semifinal Exam: Student grading system using functions and arrays

double computeAverage(int scores[], int count) {
    int sum = 0;
    for (int i = 0; i < count; i++) {
        sum += scores[i];
    }
    return (double)sum / count;
}

char determineGrade(double average) {
    if (average >= 90) {
        return 'A';
    } else if (average >= 80) {
        return 'B';
    } else if (average >= 70) {
        return 'C';
    } else if (average >= 60) {
        return 'D';
    } else {
        return 'F';
    }
}

int main() {
    int studentScores[5];

    printf("Enter 5 quiz scores:\n");
    for (int i = 0; i < 5; i++) {
        printf("Score %d: ", i + 1);
        scanf("%d", &studentScores[i]);
    }

    double finalAverage = computeAverage(studentScores, 5);
    char finalGrade = determineGrade(finalAverage);

    printf("\nAverage: %.2lf\n", finalAverage);
    printf("Grade: %c\n", finalGrade);

    return 0;
}
