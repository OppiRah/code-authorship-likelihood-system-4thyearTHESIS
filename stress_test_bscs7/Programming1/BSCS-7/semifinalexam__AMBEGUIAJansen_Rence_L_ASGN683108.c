#include <stdio.h>

double GetAvg(int arrScores[], int iN) {
    int iSum = 0, iI = 0;

    do {
        iSum += arrScores[iI];
        iI++;
    } while (iI < iN);

    return (double)iSum / iN;
}

char GetGrade(double dAvg) {
    if (dAvg >= 90) {
        return 'A';
    } else if (dAvg >= 80) {
        return 'B';
    } else if (dAvg >= 70) {
        return 'C';
    } else if (dAvg >= 60) {
        return 'D';
    } else {
        return 'F';
    }
}

int main() {
    int arrScores[5];
    int iI = 0;

    printf("Enter 5 quiz scores:\n");
    do {
        printf("Score %d: ", iI + 1);
        scanf("%d", &arrScores[iI]);
        iI++;
    } while (iI < 5);

    double dAvg = GetAvg(arrScores, 5);
    char chGrade = GetGrade(dAvg);

    printf("\nAverage: %.2lf\n", dAvg);
    printf("Grade: %c\n", chGrade);

    return 0;
}
