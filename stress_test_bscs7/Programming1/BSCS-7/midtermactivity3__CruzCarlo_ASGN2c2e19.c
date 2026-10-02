// Carlo Cruz - Leap Year

#include <stdio.h>

int main() {
    int year;
    printf("Enter year: ");
    scanf("%d", &year);

    if (year % 400 == 0) {
        puts("Leap year");
    } else if (year % 100 == 0) {
        puts("Not a leap year");
    } else if (year % 4 == 0) {
        puts("Leap year");
    } else {
        puts("Not a leap year");
    }
    return 0;
}
