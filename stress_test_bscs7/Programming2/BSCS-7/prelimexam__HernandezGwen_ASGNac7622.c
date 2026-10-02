#include <stdio.h>

int is_prime(int num)
{
	if (num < 2) {
		return 0;
	}
	for (int i = 2; i <= num / 2; i++) {
		if (num % i == 0) {
			// found a divisor
			return 0;
		}
	}
	return 1;
}

int main(void)
{
	int start, stop;
	scanf("%d %d", &start, &stop);
	int prime_count = 0, prime_sum = 0;
	int numbers[500];
	for (int i=start; i <= stop; i++) {
		if (is_prime(i)) {
			numbers[prime_count] = i;
			prime_count++;
			prime_sum += i;
		}
	}
	for (int i = 0; i < prime_count; i++) {
		printf("%d ", numbers[i]);
	}
	printf("\nCount: %d, Sum: %d\n", prime_count, prime_sum);
	int composites = 0;
	for (int i = start; i <= stop; i++) {
		if (i>1 && !is_prime(i)) {
			composites=composites + 1;
		}
	}
	printf("Composite numbers: %d\n", composites);
	int largest = -1;
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
