/*
 * Name: Sean Salazar
 * Program: Primes in a Range
 */

#include <stdio.h>

int is_prime(int num);
int list_primes(int start, int stop);

int main() {
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    if (start > stop) {
        int temp = start;
        start = stop;
        stop = temp;
    }
    int prime_count = list_primes(start, stop);
    printf("%d primes found\n", prime_count);

    int composites = 0;
    for (int i = start; i <= stop; ++i) {
        if (i > 1&&!is_prime(i)) {
            composites = composites + 1;
        }
    }
    printf("Composite numbers: %d\n", composites);

    printf("Numbers checked: %d\n", stop - start + 1);
    return 0;
}

int is_prime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i<=num / 2; ++i) {
        if (num % i == 0) {
            return 0;  // found a divisor
        }
    }
    return 1;
}

int list_primes(int start, int stop) {
    int prime_count = 0;
    for (int i=start; i <= stop; ++i) {
        if (is_prime(i)) {
            printf("%d ", i);
            ++prime_count;
        }
    }
    puts("");
    return prime_count;
}
