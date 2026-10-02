/*
 * Name: Warren Villanueva
 * Program: Even/Odd and Sign
 */

#include <stdio.h>

int main() {
    int num;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (num % 2==0) {  // check even or odd
        printf("%d is even\n", num);
    } else {
        printf("%d is odd\n", num);
    }

    if (num > 0) {  // check the sign
        printf("positive\n");
    } else if (num < 0) {
        printf("negative\n");
    } else {
        printf("zero\n");
    }
}
