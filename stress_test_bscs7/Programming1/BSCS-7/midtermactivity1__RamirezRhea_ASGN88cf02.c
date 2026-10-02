/*
 * Name: Rhea Ramirez
 * Program: Even/Odd and Sign
 */

#include <stdio.h>

int main(void)
{
    int num;
    printf("Enter a number: ");
    scanf("%d", &num);

    if (num % 2==0)
    {
        printf("%d is even\n", num);
    }
    else
    {
        printf("%d is odd\n", num);
    }

    if (num > 0)
    {
        printf("positive\n");
    }
    else if (num < 0)
    {
        printf("negative\n");
    }
    else
    {
        printf("zero\n");
    }
}
