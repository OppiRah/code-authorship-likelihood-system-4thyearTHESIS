#include <stdio.h>

int binary_search(int numbers[], int size, int key)
{
	for (int i = 0; i<size && numbers[i] <= key; i++) {
		if (numbers[i] == key) {
			return i;
		}
	}
	return -1;
}

void print_array(int numbers[], int size)
{
	for (int i = 0; i < size; i++) {
		printf("%d ", numbers[i]);
	}
	printf("\n");
}

void sort_array(int numbers[], int size)
{
	for (int i = 0; i < size - 1; i++) {
		int min_index = i;
		for (int j = i + 1; j < size; j++) {
			if (numbers[j] < numbers[min_index]) {
				min_index=j;
			}
		}
		int temp = numbers[i];
		numbers[i] = numbers[min_index];
		numbers[min_index] = temp;
	}
}

int main(void)
{
	int numbers[50], size;
	printf("How many numbers? ");
	scanf("%d", &size);
	for (int i=0; i < size; i++) {
		printf("Enter number %d: ", i + 1);
		scanf("%d", &numbers[i]);
	}
	sort_array(numbers, size);
	printf("Sorted: ");
	print_array(numbers, size);
	float median;
	if (size % 2 == 0) {
		median = (numbers[size / 2 - 1] + numbers[size / 2])/2.0;
	} else {
		median = numbers[size / 2];
	}
	printf("Median: %.1f\n", median);
	int key;
	printf("Search for: ");
	scanf("%d", &key);
	int index = binary_search(numbers, size, key);
	printf(index >= 0 ? "Found at %d\n" : "Not found\n", index);

	printf("Smallest: %d, Largest: %d\n", numbers[0], numbers[size - 1]);
	int total = 0;
	for (int i = 0; i < size; i++) {
		total += numbers[i];
	}
	printf("Sum: %d, Average: %.2f\n", total, (float)total / size);
	return 0;
}
