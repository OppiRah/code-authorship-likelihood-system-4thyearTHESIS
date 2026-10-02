// Isabel Jimenez - Sum 1 to N

#include <stdio.h>

int main() {
	int limit, sum = 0;
	printf("Enter n: ");
	scanf("%d", &limit);

	// add 1 to n
	int i = 1;
	while (i <= limit) {
		sum += i;
		++i;
	}
	printf("Sum: %d\n", sum);
	printf("Average: %.2f\n", sum / (float)limit);
	return 0;
}
