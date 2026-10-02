// Aaron Aquino - Sum 1 to N

#include <stdio.h>

int main() {
    int limit, sum = 0;
    scanf("%d", &limit);

    int i = 1;
    while (i <= limit) {
        sum = sum + i;
        ++i;
    }
    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", sum * 1.0 / limit);
    return 0;
}
