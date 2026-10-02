#include <stdio.h>

int main(void)
{
	int numbers[50], size;
	scanf("%d", &size);
	if (size <= 0 || size > 50) {
		puts("Invalid size");
		return 1;
	}
	for (int i = 0; i < size; i++) {
		scanf("%d", &numbers[i]);
	}

	int maximum = numbers[0], minimum = numbers[0], sum = 0;
	// one pass for everything
	for (int i = 0; i < size; i++) {
		sum += numbers[i];
		if (numbers[i]>maximum) {
			maximum = numbers[i];
		}
		if (numbers[i] < minimum) {
			minimum = numbers[i];
		}
	}
	float average = (float)sum / size;
	int above = 0;
	for (int i = 0; i < size; i++) {
		if (numbers[i] > average) {
			above++;
		}
	}

	printf("\n--- Results ---\n");
	printf("Max: %d\nMin: %d\nAverage: %.2f\nAbove average: %d\n", maximum, minimum, average, above);
	return 0;
}
