/*
 * Name: Sean Salazar
 * Program: Even/Odd and Sign
 */

#include <stdio.h>

int main() {
    int num;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num % 2 == 0) {  // check even or odd
        printf("%d is even\n", num);
    } else {
        printf("%d is odd\n", num);
    }
    if (num > 0) {  // check the sign
        puts("positive");
    } else if (num < 0) {
        puts("negative");
    } else {
        puts("zero");
    }
    return 0;
}
