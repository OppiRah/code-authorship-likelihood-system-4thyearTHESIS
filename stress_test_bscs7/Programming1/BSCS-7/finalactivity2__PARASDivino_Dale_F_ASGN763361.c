#include <stdio.h>

// pointer swap
void doSwap(int *a, int *b) {
    int t=*a;
    *a=*b;
    *b=t;
}

int main() {
    int x, y;

    printf("Enter first value: ");
    scanf("%d", &x);
    printf("Enter second value: ");
    scanf("%d", &y);
    printf("\nBefore: %d, %d\n", x, y);
    doSwap(&x, &y);
    printf("After: %d, %d\n", x, y);
    return 0;
}
