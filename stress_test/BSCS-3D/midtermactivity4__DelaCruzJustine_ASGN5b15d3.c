// Class activity: Temperature Converter
// Submitted by: Justine Dela Cruz

#include <stdio.h>

int main() {
    // convert celsius to fahrenheit
    float cel = 25.0;
    float fah = (cel * 9 / 5) + 32;
    printf("%.1fC = %.1fF\n", cel, fah);
    return 0;
}
