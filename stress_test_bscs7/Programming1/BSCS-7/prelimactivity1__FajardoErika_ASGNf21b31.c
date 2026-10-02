// Erika Fajardo - Rectangle Area and Perimeter

#include <stdio.h>

int main() {
    float length, width, area, perimeter;
    // get the dimensions
    printf("Enter length: ");
    scanf("%f", &length);
    printf("Enter width: ");
    scanf("%f", &width);

    area = length * width;  // area formula
    perimeter = 2 * (length + width);  // perimeter formula

    printf("Area: %f\n", area);
    printf("Perimeter: %f\n", perimeter);
    return 0;
}
