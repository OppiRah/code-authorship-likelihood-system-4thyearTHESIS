/*
 * Name: Warren Villanueva
 * Program: Primes in a Range
 */

#include <stdio.h>

int is_prime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i * i <= num; i += 1) {
        if (num % i == 0) {
            return 0;  // found a divisor
        }
    }
    return 1;
}

int main() {
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    int prime_count = 0, prime_sum = 0;
    for (int i = start; i <= stop; i += 1) {
        if (is_prime(i)) {
            printf("%d ", i);
            prime_count += 1;
            prime_sum = prime_sum + i;
        }
    }
    printf("\n--- Results ---\n");
    printf("\nCount: %d, Sum: %d\n", prime_count, prime_sum);
}
