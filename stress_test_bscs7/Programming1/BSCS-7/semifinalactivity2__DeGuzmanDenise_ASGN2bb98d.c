/*
 * Name: Denise De Guzman
 * Program: Multiplication Table
 */

#include <stdio.h>

int main() {
    int num;
    printf("Enter a number: ");
    scanf("%d", &num);
    for (int i = 1; i<=10; i++) {  // print the table up to 10
        printf("%d x %d = %d\n", num, i, num * i);
    }
    return 0;
}
