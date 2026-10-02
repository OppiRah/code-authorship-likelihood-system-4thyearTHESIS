// Aaron Aquino - Factorial

#include <stdio.h>

int main() {
    int num;
    long long factorial = 1;
    scanf("%d", &num);

    if (num < 0) {
        printf("No factorial for negatives\n");
    } else {
        for (int i = 2; i <= num; ++i) {
            factorial = factorial * i;  // multiply up
        }
        printf("%d! = %lld\n", num, factorial);
    }
    return 0;
}
