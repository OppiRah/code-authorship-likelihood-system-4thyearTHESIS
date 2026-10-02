// Erika Fajardo - Primes in a Range

#include <stdio.h>

int isPrime(int num);

int main() {
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    int primeCount = 0, primeSum = 0;
    for (int i = start; i <= stop; ++i) {
        if (isPrime(i)) {
            printf("%d ", i);
            ++primeCount;
            primeSum = primeSum + i;
        }
    }
    printf("\nCount: %d, Sum: %d\n", primeCount, primeSum);
    int largest = -1;
    for (int i = stop; i >= start; i--) {
        if (isPrime(i)) {
            largest = i;
            break;
        }
    }
    printf("Largest prime: %d\n", largest);
    return 0;
}

int isPrime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i * i <= num; ++i) {
        if (num % i==0) {
            return 0;
        }
    }
    return 1;
}
