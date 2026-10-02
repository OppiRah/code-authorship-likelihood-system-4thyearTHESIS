#include <stdio.h>

int main(void)
{
	int numbers[50], size;
	printf("How many numbers? ");
	scanf("%d", &size);
	for (int i = 0; i < size; i++) {
		printf("Enter number %d: ", i + 1);
		scanf("%d", &numbers[i]);
	}
	int sum = 0;

	for (int i = 0; i < size; i++) {
		sum += numbers[i];
	}
	printf("Sum = %d\n", sum);
	printf("Average = %.2f\n", (float)sum/size);
	return 0;
}
