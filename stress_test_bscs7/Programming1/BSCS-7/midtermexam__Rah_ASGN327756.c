#include <stdio.h>

int main() {
    // Midterm Exam: Array statistics program
    // Computes sum, average, maximum, and minimum of an array

    int scoreArray[30];
    int totalScores;

    printf("Enter how many scores to input: ");
    scanf("%d", &totalScores);

    for (int i = 0; i < totalScores; i++) {
        printf("Enter score %d: ", i + 1);
        scanf("%d", &scoreArray[i]);
    }

    int sumOfScores = 0;
    int maxScore = scoreArray[0];
    int minScore = scoreArray[0];

    // Single pass to compute sum, max, and min together
    for (int i = 0; i < totalScores; i++) {
        sumOfScores += scoreArray[i];

        if (scoreArray[i] > maxScore) {
            maxScore = scoreArray[i];
        }
        if (scoreArray[i] < minScore) {
            minScore = scoreArray[i];
        }
    }

    double averageScore = (double)sumOfScores / totalScores;

    printf("\nSum: %d\n", sumOfScores);
    printf("Average: %.2lf\n", averageScore);
    printf("Maximum: %d\n", maxScore);
    printf("Minimum: %d\n", minScore);

    return 0;
}
