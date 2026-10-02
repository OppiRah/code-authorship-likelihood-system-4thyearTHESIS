// Faith Flores - Primes in a Range

#include <stdio.h>

int isPrime(int num);
int listPrimes(int start, int stop);

int main() {
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    if (start > stop) {
        int temp = start;
        start = stop;
        stop = temp;
    }
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
    for (int i = start; i + 2 <= stop; i += 1) {
        if (isPrime(i) && isPrime(i + 2)) {
            twins+=1;
        }
    }
    printf("Twin prime pairs: %d\n", twins);
    return 0;
}

int isPrime(int num) {
    if (num < 2) {
        return 0;
    }
    for (int i = 2; i <= num/2; i += 1) {
        if (num % i == 0) {
            return 0;
        }
    }
    return 1;
}

int listPrimes(int start, int stop) {
    int primeCount = 0;
    for (int i = start; i <= stop; i += 1) {
        if (isPrime(i)) {
            printf("%d ", i);
            primeCount += 1;
        }
    }
    printf("\n");
    return primeCount;
}
