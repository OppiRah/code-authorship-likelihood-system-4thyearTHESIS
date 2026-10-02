#include <stdio.h>

// prime checker
int isPrime(int n) {
    if (n<2) return 0;
    for (int d=2; d*d<=n; d++) {
        if (n%d==0) return 0;
    }
    return 1;
}

int main() {
    int num;

    printf("Enter a number: ");
    scanf("%d", &num);
    printf(isPrime(num) ? "\n%d is prime.\n" : "\n%d is not prime.\n", num);
    return 0;
}
