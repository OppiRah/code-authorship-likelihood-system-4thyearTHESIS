// Practice exercise: Fibonacci Sequence
// Submitted by: Ricardo Lazaro

#include <stdio.h>

int main() {
    // print first N fibonacci numbers
    int count = 10;
    int a = 0, b = 1;
    for (int i = 0; i < count; i++) {
        printf("%d ", a);
        int next = a + b;
        a = b;
        b = next;
    }
    printf("\n");
    return 0;
}
