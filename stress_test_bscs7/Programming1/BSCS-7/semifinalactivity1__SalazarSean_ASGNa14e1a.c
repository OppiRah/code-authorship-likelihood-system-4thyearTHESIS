/*
 * Name: Sean Salazar
 * Program: Sum 1 to N
 */

#include <stdio.h>

int main() {
    int limit, sum = 0;
    printf("Enter n: ");
    scanf("%d", &limit);
    for (int i = 1; i <= limit; ++i) {  // add 1 to n
        sum = sum + i;
    }
    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", sum / (float)limit);
    return 0;
}
