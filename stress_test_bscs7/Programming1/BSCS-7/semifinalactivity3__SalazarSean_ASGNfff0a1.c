/*
 * Name: Sean Salazar
 * Program: Factorial
 */

#include <stdio.h>

int main() {
    int num;
    long long factorial = 1;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num < 0) {
        puts("No factorial for negatives");
    } else {
        for (int i = 2; i <= num; ++i) {
            factorial = factorial * i;  // multiply up
        }
        printf("%d! = %lld\n", num, factorial);
    }
    return 0;
}
