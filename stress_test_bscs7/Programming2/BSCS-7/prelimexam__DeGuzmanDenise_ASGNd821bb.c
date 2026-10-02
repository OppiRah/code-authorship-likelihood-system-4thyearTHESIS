/*
 * Name: Denise De Guzman
 * Program: Primes in a Range
 */

#include <stdio.h>

int is_prime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i < num; i++) {
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
    int prime_count = 0, prime_sum=0;
    for (int i = start; i <= stop; i++) {
        if (is_prime(i)) {
            printf("%d ", i);
            prime_count++;
            prime_sum = prime_sum + i;
        }
    }
    printf("\nCount: %d, Sum: %d\n", prime_count, prime_sum);

    int composites = 0;
    for (int i = start; i <= stop; i++) {
        if (i > 1 && !is_prime(i)) {
            composites = composites + 1;
        }
    }
    printf("Composite numbers: %d\n", composites);

    int largest=-1;
    for (int i = stop; i >= start; i--) {
        if (is_prime(i)) {
            largest = i;
            break;
        }
    }
    printf("\n--- Results ---\n");
    printf("Largest prime: %d\n", largest);
    return 0;
}
