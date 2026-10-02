// Carlo Cruz - Pointer Traversal

#include <stdio.h>

int main() {
    int numbers[] = {12, 7, 31, 4, 19, 26};
    int *ptr = numbers;
    int *endPtr = numbers + 6;
    int sum = 0, maximum = *ptr;
    while (ptr < endPtr) {
        sum += *ptr;
        if (*ptr > maximum) {
            maximum = *ptr;
        }
        ptr++;
    }
    printf("Sum: %d, Max: %d\n", sum, maximum);
    return 0;
}
