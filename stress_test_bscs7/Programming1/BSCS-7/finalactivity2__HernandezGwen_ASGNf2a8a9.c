#include <stdio.h>

int main(void)
{
	int numbers[50], size;
	scanf("%d", &size);
	for (int i = 0; i < size; i++) {
		scanf("%d", &numbers[i]);
	}
	int maximum = numbers[0], position = 0;

	for (int i=1; i < size; i++) {
		if (numbers[i] > maximum) {
			maximum = numbers[i];
			// remember where
			position = i;
		}
	}
	printf("Max = %d at index %d\n", maximum, position);
	return 0;
}
