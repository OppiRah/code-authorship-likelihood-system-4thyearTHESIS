#include <stdio.h>

int IsPrime(int iN) {
    if (iN < 2) {
        return 0;
    }

    int iD = 2;
    do {
        if (iN % iD == 0) {
            return 0;
        }
        iD++;
    } while (iD * iD <= iN);

    return 1;
}

int main() {
    int iNum;

    printf("Enter a number: ");
    scanf("%d", &iNum);

    if (IsPrime(iNum)) {
        printf("\n%d is prime.\n", iNum);
    } else {
        printf("\n%d is not prime.\n", iNum);
    }

    return 0;
}
