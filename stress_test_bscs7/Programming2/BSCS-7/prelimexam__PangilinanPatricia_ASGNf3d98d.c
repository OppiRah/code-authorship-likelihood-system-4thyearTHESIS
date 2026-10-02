// Patricia Pangilinan - Primes in a Range

#include <stdio.h>

int isPrime(int num);

int main() {
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    if (start > stop) {
        int temp = start;
        start = stop;
        stop = temp;
    }
    int primeCount = 0, primeSum = 0;
    int numbers[500];
    for (int i = start; i <= stop; ++i) {
        if (isPrime(i)) {
            numbers[primeCount] = i;
            ++primeCount;
            primeSum += i;
        }
    }
    for (int i = 0; i < primeCount; ++i) {
        printf("%d ", numbers[i]);
    }
    printf("\nCount: %d, Sum: %d\n", primeCount, primeSum);

    int composites = 0;
    for (int i = start; i <= stop; ++i) {
        if (i > 1 && !isPrime(i)) {
            composites = composites + 1;
        }
    }
    printf("Composite numbers: %d\n", composites);
    return 0;
}

int isPrime(int num) {
    if (num<2) {
        return 0;
    }
    for (int i = 2; i < num; ++i) {
        if (num % i == 0) {
            return 0;
        }
    }
    return 1;
}
