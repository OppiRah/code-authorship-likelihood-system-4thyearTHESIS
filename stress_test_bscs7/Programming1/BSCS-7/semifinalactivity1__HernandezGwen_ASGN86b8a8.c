#include <stdio.h>

int main(void)
{
	int limit, sum = 0;
	scanf("%d", &limit);

	int i = 1;
	while (i <= limit) {
		sum += i;
		i++;
	}
	printf("Sum: %d\n", sum);
	printf("Average: %.2f\n", (float)sum / limit);
	return 0;
}
