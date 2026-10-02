/*
 * Name: Francis Galang
 * Program: Rectangle Area and Perimeter
 */

#include <stdio.h>

int main(void)
{
    float length, width, area, perimeter;

    // get the dimensions
    printf("Enter length: ");
    scanf("%f", &length);
    printf("Enter width: ");
    scanf("%f", &width);
    // area formula
    area = length * width;
    // perimeter formula
    perimeter = 2 * (length + width);

    printf("Area: %f\n", area);
    printf("Perimeter: %f\n", perimeter);
}
