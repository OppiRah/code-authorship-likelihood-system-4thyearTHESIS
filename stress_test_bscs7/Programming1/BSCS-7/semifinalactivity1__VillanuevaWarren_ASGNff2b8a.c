/*
 * Name: Warren Villanueva
 * Program: Sum 1 to N
 */

#include <stdio.h>

int main() {
    int limit, sum = 0;
    printf("Enter n: ");
    scanf("%d", &limit);
    for (int i=1; i <= limit; i += 1) {  // add 1 to n
        sum = sum + i;
    }
    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", sum/(float)limit);
}
