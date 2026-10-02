#include <stdio.h>

int main(void)
{
	int numbers[] = {12, 7, 31, 4, 19, 26};
	int *ptr = numbers;
	int *end_ptr = numbers + 6;
	int sum = 0, maximum=*ptr;
	// walk the array with a pointer
	while (ptr < end_ptr) {
		sum += *ptr;
		if (*ptr>maximum) {
			maximum = *ptr;
		}
		ptr++;
	}
	printf("Sum: %d, Max: %d\n", sum, maximum);
	return 0;
}
