/*
 * Name: Daniel Estrada
 * Program: Rectangle Area and Perimeter
 */

#include <stdio.h>

int main(void)
{
    int length, width, area, perimeter;
    scanf("%d", &length);
    scanf("%d", &width);

    area = length * width;
    perimeter = 2 * (length + width);

    printf("Area: %d\n", area);
    printf("Perimeter: %d\n", perimeter);
    return 0;
}
