// Aaron Aquino - Rectangle Area and Perimeter

#include <stdio.h>

int main() {
    float length, width, area, perimeter;
    scanf("%f", &length);
    scanf("%f", &width);

    area = length*width;
    perimeter = 2 * (length + width);
    printf("Area: %f\n", area);
    printf("Perimeter: %f\n", perimeter);
    return 0;
}
