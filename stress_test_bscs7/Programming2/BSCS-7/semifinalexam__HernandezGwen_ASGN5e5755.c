#include <stdio.h>

void print_array(int numbers[], int size)
{
	for (int i = 0; i < size; i++) {
		printf("%d ", numbers[i]);
	}
	puts("");
}

void sort_array(int numbers[], int size)
{
	for (int i = 0; i < size - 1; i++) {
		int min_index = i;
		for (int j = i + 1; j < size; j++) {
			if (numbers[j] < numbers[min_index]) {
				min_index = j;
			}
		}
		int temp = numbers[i];
		numbers[i] = numbers[min_index];
		numbers[min_index] = temp;
	}
}

int binary_search(int numbers[], int size, int key)
{
	// array is sorted so we can stop early
	for (int i=0; i < size && numbers[i] <= key; i++) {
		if (numbers[i] == key) {
			return i;
		}
	}
	return -1;
}

int main(void)
{
	int numbers[50], size;
	scanf("%d", &size);
	for (int i = 0; i < size; i++) {
		scanf("%d", &numbers[i]);
	}
	sort_array(numbers, size);
	printf("Sorted: ");
	print_array(numbers, size);
	float median=(size % 2) ? numbers[size / 2] : (numbers[size/2 - 1] + numbers[size / 2]) / 2.0;
	printf("Median: %.1f\n", median);
	int key;
	scanf("%d", &key);
	int index = binary_search(numbers, size, key);
	printf(index >= 0 ? "Found at %d\n" : "Not found\n", index);
	int total = 0;
	for (int i = 0; i < size; i++) {
		total+=numbers[i];
	}
	printf("Sum: %d, Average: %.2f\n", total, (float)total / size);

	int duplicates = 0;
	for (int i=1; i < size; i++) {
		if (numbers[i] == numbers[i - 1]) {
			duplicates++;
		}
	}
	printf("Duplicates: %d\n", duplicates);
	return 0;
}
