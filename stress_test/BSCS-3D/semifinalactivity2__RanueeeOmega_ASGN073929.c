#include <stdio.h>

int is_prime(int n) {
    if (n < 2) return 0;
    for (int d = 2; d*d <= n; d++) {
        if (n%d == 0) return 0;
    }
    return 1;
}

int main() {
    int num;
    printf("Enter a number: ");
    scanf("%d", &num);
    if (is_prime(num)) printf("\n%d is prime.\n", num);
    else printf("\n%d is not prime.\n", num);
    return 0;
}
