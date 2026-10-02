// Aaron Aquino - Primes in a Range

#include <stdio.h>

int isPrime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) {
            return 0;  // found a divisor
        }
    }
    return 1;
}

int listPrimes(int start, int stop) {
    int primeCount = 0;
    for (int i = start; i <= stop; ++i) {
        if (isPrime(i)) {
            printf("%d ", i);
            ++primeCount;
        }
    }
    printf("\n");
    return primeCount;
}

int main() {
    int start, stop;
    scanf("%d %d", &start, &stop);
    int primeCount = listPrimes(start, stop);
    printf("%d primes found\n", primeCount);
    int largest = -1;
    for (int i = stop; i >= start; i--) {
        if (isPrime(i)) {
            largest = i;
            break;
        }
    }
    printf("Largest prime: %d\n", largest);

    int twins = 0;
    for (int i = start; i + 2 <= stop; ++i) {
        if (isPrime(i) && isPrime(i + 2)) {
            ++twins;
        }
    }
    printf("Twin prime pairs: %d\n", twins);
    return 0;
}
