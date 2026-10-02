#include <stdio.h>

int main() {
    int arrNums[20];
    int iCount, iIdx = 0;

    printf("How many to sort? ");
    scanf("%d", &iCount);

    do {
        printf("Number %d: ", iIdx + 1);
        scanf("%d", &arrNums[iIdx]);
        iIdx++;
    } while (iIdx < iCount);

    int iPass = 0;
    do {
        int iJ = 0;
        do {
            if (arrNums[iJ] > arrNums[iJ + 1]) {
                int iTemp = arrNums[iJ]; // swap
                arrNums[iJ] = arrNums[iJ + 1];
                arrNums[iJ + 1] = iTemp;
            }
            iJ++;
        } while (iJ < iCount - iPass - 1);
        iPass++;
    } while (iPass < iCount - 1);

    printf("\nSorted: ");
    iIdx = 0;
    do {
        printf("%d ", arrNums[iIdx]);
        iIdx++;
    } while (iIdx < iCount);
    printf("\n");

    return 0;
}
