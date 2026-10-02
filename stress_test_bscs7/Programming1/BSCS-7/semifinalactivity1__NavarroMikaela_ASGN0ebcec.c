/*
 * Name: Mikaela Navarro
 * Program: Sum 1 to N
 */

#include <stdio.h>

int main() {
    int limit, sum = 0;
    printf("Enter n: ");
    scanf("%d", &limit);
    int i = 1;  // add 1 to n
    while (i <= limit) {
        sum = sum + i;
        i++;
    }
    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", (float)sum / limit);
    return 0;
}
