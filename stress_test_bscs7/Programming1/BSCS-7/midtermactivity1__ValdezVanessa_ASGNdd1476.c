/*
 * Name: Vanessa Valdez
 * Program: Even/Odd and Sign
 */

#include <stdio.h>

int main(void)
{
    int num;
    printf("Enter a number: ");
    scanf("%d", &num);

    // check even or odd
    if (num % 2 == 0)
    {
        printf("%d is even\n", num);
    }
    else
    {
        printf("%d is odd\n", num);
    }

    // check the sign
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
    return 0;
}
