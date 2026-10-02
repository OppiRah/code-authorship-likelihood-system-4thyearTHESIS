/*
 * Name: Denise De Guzman
 * Program: Rectangle Area and Perimeter
 */

#include <stdio.h>

int main() {
    int length, width, area, perimeter;

    printf("Enter length: ");  // get the dimensions
    scanf("%d", &length);
    printf("Enter width: ");
    scanf("%d", &width);
    area = length * width;  // area formula
    perimeter = 2 * (length+width);  // perimeter formula

    printf("Area: %d\n", area);
    printf("Perimeter: %d\n", perimeter);
    return 0;
}
