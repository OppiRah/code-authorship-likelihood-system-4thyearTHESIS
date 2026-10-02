/*
 * Name: Rhea Ramirez
 * Program: Rectangle Area and Perimeter
 */

#include <stdio.h>

int main(void)
{
    int length, width, area, perimeter;
    // get the dimensions
    printf("Enter length: ");
    scanf("%d", &length);
    printf("Enter width: ");
    scanf("%d", &width);

    area = length * width;
    // perimeter formula
    perimeter = 2 * (length + width);

    printf("Area: %d\n", area);
    printf("Perimeter: %d\n", perimeter);
}
