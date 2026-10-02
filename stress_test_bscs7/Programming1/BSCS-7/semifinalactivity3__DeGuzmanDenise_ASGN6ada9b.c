/*
 * Name: Denise De Guzman
 * Program: Factorial
 */

#include <stdio.h>

int main() {
    int num;
    long long factorial = 1;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num < 0) {
        printf("No factorial for negatives\n");
    } else {
        int i = num;
        while (i > 1) {
            factorial = factorial * i;
            i--;
        }
        printf("%d! = %lld\n", num, factorial);
    }
    return 0;
}
