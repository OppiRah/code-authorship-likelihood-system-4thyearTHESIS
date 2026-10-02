/*
 * Name: Rhea Ramirez
 * Program: Leap Year
 */

#include <stdio.h>

int main(void)
{
    int year;
    printf("Enter year: ");
    scanf("%d", &year);
    if (year%400 == 0)
    {
        printf("Leap year\n");
    }
    else if (year % 100 == 0)
    {
        printf("Not a leap year\n");
    }
    else if (year % 4 == 0)
    {
        printf("Leap year\n");
    }
    else
    {
        printf("Not a leap year\n");
    }
}
