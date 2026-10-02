/*
 * Name: Mikaela Navarro
 * Program: Primes in a Range
 */

#include <stdio.h>

int is_prime(int num);
int list_primes(int start, int stop);

int main() {
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    int prime_count = list_primes(start, stop);
    printf("\n--- Results ---\n");
    printf("%d primes found\n", prime_count);
    return 0;
}

int is_prime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) {
            return 0;  // found a divisor
        }
    }
    return 1;
}

int list_primes(int start, int stop) {
    int prime_count=0;
    for (int i = start; i<=stop; i++) {
        if (is_prime(i)) {
            printf("%d ", i);
            prime_count++;
        }
    }
    printf("\n");
    return prime_count;
}
