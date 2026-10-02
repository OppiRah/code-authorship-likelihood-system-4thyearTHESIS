/*
 * Name: Francis Galang
 * Program: Factorial
 */

#include <stdio.h>

int main(void)
{
    int num;
    long long factorial = 1;
    printf("Enter a number: ");
    scanf("%d", &num);

    if (num < 0)
    {
        printf("No factorial for negatives\n");
    }
    else
    {
        for (int i = 2; i <= num; i++)
        {
            // multiply up
            factorial = factorial * i;
        }
        printf("%d! = %lld\n", num, factorial);
    }
}
