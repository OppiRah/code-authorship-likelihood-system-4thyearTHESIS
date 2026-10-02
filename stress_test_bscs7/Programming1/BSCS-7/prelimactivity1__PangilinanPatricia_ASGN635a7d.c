// Patricia Pangilinan - Rectangle Area and Perimeter

#include <stdio.h>

int main() {
    int length, width, area, perimeter;
    printf("Enter length: ");
    scanf("%d", &length);
    printf("Enter width: ");
    scanf("%d", &width);

    area = length * width;
    perimeter = 2 * (length + width);

    printf("Area: %d\n", area);
    printf("Perimeter: %d\n", perimeter);
    return 0;
}
