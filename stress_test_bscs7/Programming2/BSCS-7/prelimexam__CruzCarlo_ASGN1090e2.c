// Carlo Cruz - Primes in a Range

#include <stdio.h>

int isPrime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i <= num / 2; i++) {
        if (num % i == 0) {
            return 0;
        }
    }
    return 1;
}

int listPrimes(int start, int stop) {
    int primeCount = 0;
    for (int i = start; i <= stop; i++) {
        if (isPrime(i)) {
            printf("%d ", i);
            primeCount++;
        }
    }
    puts("");
    return primeCount;
}

int main() {
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    int primeCount = listPrimes(start, stop);
    printf("%d primes found\n", primeCount);

    int composites = 0;
    for (int i = start; i <= stop; i++) {
        if (i > 1 && !isPrime(i)) {
            composites = composites + 1;
        }
    }
    printf("Composite numbers: %d\n", composites);

    int twins = 0;
    for (int i = start; i + 2 <= stop; i++) {
        if (isPrime(i) && isPrime(i + 2)) {
            twins++;
        }
    }
    printf("Twin prime pairs: %d\n", twins);

    int largest = -1;
    for (int i = stop; i >= start; i--) {
        if (isPrime(i)) {
            largest = i;
            break;
        }
    }
    printf("Largest prime: %d\n", largest);

    printf("Numbers checked: %d\n", stop - start + 1);
    return 0;
}
