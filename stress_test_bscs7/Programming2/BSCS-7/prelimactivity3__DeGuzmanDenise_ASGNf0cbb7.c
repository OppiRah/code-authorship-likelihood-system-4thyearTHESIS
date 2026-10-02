/*
 * Name: Denise De Guzman
 * Program: Swap with Pointers
 */

#include <stdio.h>

void swap(int *first, int *second) {
    int temp = *first;
    *first = *second;
    *second = temp;
}

int main() {
    int num1, num2;
    printf("Enter two numbers: ");
    scanf("%d %d", &num1, &num2);
    printf("Before: %d %d\n", num1, num2);
    swap(&num1, &num2);
    printf("After: %d %d\n", num1, num2);
    return 0;
}
