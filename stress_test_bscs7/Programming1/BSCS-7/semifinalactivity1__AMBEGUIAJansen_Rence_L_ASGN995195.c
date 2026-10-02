#include <stdio.h>

double AddNums(double dA, double dB) {
    return dA + dB;
}

double SubNums(double dA, double dB) {
    return dA - dB;
}

double MulNums(double dA, double dB) {
    return dA * dB;
}

int main() {
    double dX, dY;

    printf("Enter first number: ");
    scanf("%lf", &dX);
    printf("Enter second number: ");
    scanf("%lf", &dY);

    printf("\nSum: %.2lf\n", AddNums(dX, dY));
    printf("Diff: %.2lf\n", SubNums(dX, dY));
    printf("Product: %.2lf\n", MulNums(dX, dY));

    return 0;
}
