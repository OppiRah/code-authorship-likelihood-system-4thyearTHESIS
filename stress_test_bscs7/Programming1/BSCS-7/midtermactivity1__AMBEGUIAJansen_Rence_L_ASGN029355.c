#include <stdio.h>

int main() {
    int iNum, iMultiplier = 1;

    printf("Enter a number: ");
    scanf("%d", &iNum);

    printf("\nMultiplication table:\n");

    do {
        printf("%d x %d = %d\n", iNum, iMultiplier, iNum * iMultiplier);
        iMultiplier++;
    } while (iMultiplier <= 10); // table goes up to 10

    return 0;
}
