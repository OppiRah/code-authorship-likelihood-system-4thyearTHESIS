// Isabel Jimenez - Primes in a Range

#include <stdio.h>

int isPrime(int num) {
	if (num < 2) {
		return 0;
	}
	for (int i = 2; i * i <= num; ++i) {
		if (num % i == 0) {
			// found a divisor
			return 0;
		}
	}
	return 1;
}

int main() {
	int start, stop;
	printf("Enter range (start end): ");
	scanf("%d %d", &start, &stop);
	int primeCount = 0, primeSum = 0;
	int numbers[500];
	for (int i = start; i <= stop; ++i) {
		if (isPrime(i)) {
			numbers[primeCount] = i;
			++primeCount;
			primeSum += i;
		}
	}
	for (int i = 0; i<primeCount; ++i) {
		printf("%d ", numbers[i]);
	}
	printf("\nCount: %d, Sum: %d\n", primeCount, primeSum);

	printf("Numbers checked: %d\n", stop - start+1);
	int composites = 0;
	for (int i = start; i <= stop; ++i) {
		if (i > 1 && !isPrime(i)) {
			composites = composites + 1;
		}
	}
	printf("Composite numbers: %d\n", composites);
	int twins = 0;
	for (int i = start; i + 2 <= stop; ++i) {
		if (isPrime(i) && isPrime(i + 2)) {
			++twins;
		}
	}
	printf("\n--- Results ---\n");
	printf("Twin prime pairs: %d\n", twins);
	return 0;
}
