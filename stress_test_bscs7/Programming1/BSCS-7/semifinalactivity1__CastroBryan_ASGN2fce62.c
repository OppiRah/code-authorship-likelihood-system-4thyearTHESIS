/*
 * Name: Bryan Castro
 * Program: Sum 1 to N
 */

#include <stdio.h>

int main(void)
{
    int limit, sum = 0;
    printf("Enter n: ");
    scanf("%d", &limit);

    // add 1 to n
    int i = 1;
    while (i <= limit)
    {
        sum += i;
        ++i;
    }
    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", (float)sum/limit);
    return 0;
}
