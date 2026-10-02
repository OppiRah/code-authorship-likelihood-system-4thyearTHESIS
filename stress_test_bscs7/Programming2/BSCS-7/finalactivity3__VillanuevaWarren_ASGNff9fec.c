/*
 * Name: Warren Villanueva
 * Program: Pointer Traversal
 */

#include <stdio.h>

int main() {
    int numbers[]={12, 7, 31, 4, 19, 26};
    int *ptr = numbers;
    int *end_ptr = numbers + 6;
    int sum = 0, maximum = *ptr;
    while (ptr<end_ptr) {  // walk the array with a pointer
        sum = sum + *ptr;
        if (*ptr > maximum) {
            maximum=*ptr;
        }
        ptr += 1;
    }
    printf("Sum: %d, Max: %d\n", sum, maximum);
}
