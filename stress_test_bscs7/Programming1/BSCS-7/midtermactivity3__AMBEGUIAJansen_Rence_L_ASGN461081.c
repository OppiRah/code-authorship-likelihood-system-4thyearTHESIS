#include <stdio.h>

int main() {
    int arrNums[20];
    int iCount, iIdx = 0;

    printf("How many numbers? ");
    scanf("%d", &iCount);

    do {
        printf("Number %d: ", iIdx + 1);
        scanf("%d", &arrNums[iIdx]);
        iIdx++;
    } while (iIdx < iCount);

    int iHigh = arrNums[0], iLow = arrNums[0];
    iIdx = 1;

    do {
        if (arrNums[iIdx] > iHigh) {
            iHigh = arrNums[iIdx];
        }
        if (arrNums[iIdx] < iLow) {
            iLow = arrNums[iIdx];
        }
        iIdx++;
    } while (iIdx < iCount);

    printf("\nHighest: %d\n", iHigh);
    printf("Lowest: %d\n", iLow);

    return 0;
}
