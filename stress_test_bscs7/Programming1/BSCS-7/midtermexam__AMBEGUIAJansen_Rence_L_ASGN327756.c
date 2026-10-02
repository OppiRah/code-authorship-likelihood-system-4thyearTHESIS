#include <stdio.h>

int main() {
    int arrScores[30];
    int iCount, iIdx = 0;

    printf("How many scores? ");
    scanf("%d", &iCount);

    do {
        printf("Score %d: ", iIdx + 1);
        scanf("%d", &arrScores[iIdx]);
        iIdx++;
    } while (iIdx < iCount);

    int iSum = 0, iHigh = arrScores[0], iLow = arrScores[0];
    iIdx = 0;

    do {
        iSum += arrScores[iIdx];
        if (arrScores[iIdx] > iHigh) {
            iHigh = arrScores[iIdx];
        }
        if (arrScores[iIdx] < iLow) {
            iLow = arrScores[iIdx];
        }
        iIdx++;
    } while (iIdx < iCount);

    double dAvg = (double)iSum / iCount;

    printf("\nSum: %d\n", iSum);
    printf("Average: %.2lf\n", dAvg);
    printf("Highest: %d\n", iHigh);
    printf("Lowest: %d\n", iLow);

    return 0;
}
