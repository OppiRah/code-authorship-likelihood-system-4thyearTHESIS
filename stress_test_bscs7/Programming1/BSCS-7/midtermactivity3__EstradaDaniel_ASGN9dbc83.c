/*
 * Name: Daniel Estrada
 * Program: Leap Year
 */

#include <stdio.h>

int main(void)
{
    int year;
    scanf("%d", &year);
    // leap year rule
    if (year % 400 == 0)
    {
        puts("Leap year");
    }
    else if (year % 100 == 0)
    {
        puts("Not a leap year");
    }
    else if (year % 4 == 0)
    {
        puts("Leap year");
    }
    else
    {
        puts("Not a leap year");
    }
    return 0;
}
