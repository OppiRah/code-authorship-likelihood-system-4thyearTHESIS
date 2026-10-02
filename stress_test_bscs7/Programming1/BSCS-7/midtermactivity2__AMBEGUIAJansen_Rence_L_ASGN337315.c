#include <stdio.h>

int main() {
    int iNum, iSum = 0, iTemp;

    printf("Enter a positive integer: ");
    scanf("%d", &iNum);

    iTemp = iNum;

    do {
        iSum += iTemp % 10; // grab last digit
        iTemp /= 10;
    } while (iTemp > 0);

    printf("\nSum of digits: %d\n", iSum);

    return 0;
}
