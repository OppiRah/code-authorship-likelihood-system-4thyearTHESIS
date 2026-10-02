#include <stdio.h>

int main(void)
{
	int numbers[50], size;
	scanf("%d", &size);
	for (int i = 0; i < size; i++) {
		scanf("%d", &numbers[i]);
	}
	int even_count = 0, odd_count = 0;

	for (int i = 0; i < size; i++) {
		if (numbers[i]%2 == 0) {
			even_count++;
		} else {
			odd_count++;
		}
	}
	printf("Even: %d, Odd: %d\n", even_count, odd_count);
	return 0;
}
