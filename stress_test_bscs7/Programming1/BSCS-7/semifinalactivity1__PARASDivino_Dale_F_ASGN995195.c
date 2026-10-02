#include <stdio.h>

// calc functions
double addNum(double a, double b) { return a+b; }
double subNum(double a, double b) { return a-b; }
double mulNum(double a, double b) { return a*b; }

int main() {
    double x, y;

    printf("Enter first number: ");
    scanf("%lf", &x);
    printf("Enter second number: ");
    scanf("%lf", &y);
    printf("\nSum: %.2lf\n", addNum(x,y));
    printf("Diff: %.2lf\n", subNum(x,y));
    printf("Product: %.2lf\n", mulNum(x,y));
    return 0;
}
