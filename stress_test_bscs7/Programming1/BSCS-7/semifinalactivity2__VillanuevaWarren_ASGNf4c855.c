/*
 * Name: Warren Villanueva
 * Program: Multiplication Table
 */

#include <stdio.h>

int main() {
    int num;
    printf("Enter a number: ");
    scanf("%d", &num);

    for (int i = 1; i <= 10; i += 1) {
        printf("%d x %d = %d\n", num, i, num * i);
    }
}
