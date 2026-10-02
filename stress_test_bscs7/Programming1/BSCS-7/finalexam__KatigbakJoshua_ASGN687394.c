#include <stdio.h>

int find_max(int numbers[], int size)
{
	int best = numbers[0];
	for (int i = 1; i < size; i++) {
		if (numbers[i] > best) {
			best = numbers[i];
		}
	}
	return best;
}

float compute_average(int numbers[], int size)
{
	int sum = 0;
	for (int i = 0; i < size; i++) {
		sum += numbers[i];
	}
	return (float)sum / size;
}

int find_min(int numbers[], int size)
{
	int best = numbers[0];
	for (int i = 1; i < size; i++) {
		if (numbers[i] < best) {
			best = numbers[i];
		}
	}
	return best;
}

int main(void)
{
	int numbers[50], size;
	printf("How many numbers? ");
	scanf("%d", &size);
	for (int i = 0; i < size; i++) {
		printf("Enter number %d: ", i+1);
		scanf("%d", &numbers[i]);
	}

	int maximum = find_max(numbers, size);
	int minimum = find_min(numbers, size);
	float average = compute_average(numbers, size);
	printf("\n--- Results ---\n");
	printf("Max: %d\nMin: %d\nAverage: %.2f\n", maximum, minimum, average);
	return 0;
}
