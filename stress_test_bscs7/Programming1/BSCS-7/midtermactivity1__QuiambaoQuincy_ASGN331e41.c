/*
 * Name: Quincy Quiambao
 * Program: Even/Odd and Sign
 */

#include <stdio.h>

int main(void)
{
    int num;
    printf("Enter a number: ");
    scanf("%d", &num);

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
        puts("positive");
    }
    else if (num < 0)
    {
        puts("negative");
    }
    else
    {
        puts("zero");
    }
    return 0;
}
