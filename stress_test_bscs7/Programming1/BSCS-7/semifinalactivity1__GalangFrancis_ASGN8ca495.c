/*
 * Name: Francis Galang
 * Program: Sum 1 to N
 */

#include <stdio.h>

int main(void)
{
    int limit, sum=0;
    printf("Enter n: ");
    scanf("%d", &limit);
    for (int i = 1; i <= limit; i++)
    {
        sum = sum + i;
    }
    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", sum * 1.0 / limit);
}
