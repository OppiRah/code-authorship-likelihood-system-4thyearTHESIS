// Erika Fajardo - Sum 1 to N

#include <stdio.h>

int main() {
    int limit, sum = 0;
    printf("Enter n: ");
    scanf("%d", &limit);
    // add 1 to n
    for (int i = 1; i <= limit; ++i) {
        sum = sum + i;
    }
    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", sum * 1.0 / limit);
    return 0;
}
