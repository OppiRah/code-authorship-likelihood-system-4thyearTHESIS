#include <stdio.h>

double add(double a, double b) { return a+b; }
double sub(double a, double b) { return a-b; }
double mul(double a, double b) { return a*b; }

int main() {
    double x, y;
    printf("Enter first number: ");
    scanf("%lf", &x);
    printf("Enter second number: ");
    scanf("%lf", &y);
    printf("\nSum: %.2lf\n", add(x,y));
    printf("Diff: %.2lf\n", sub(x,y));
    printf("Product: %.2lf\n", mul(x,y));
    return 0;
}
