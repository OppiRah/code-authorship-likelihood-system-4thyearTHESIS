#include <stdio.h>

int main(void)
{
	float value, total = 0, average;
	char letter;

	// input
	int i=0;
	while (i < 4) {
		scanf("%f", &value);
		total = total + value;
		i++;
	}
	average = total / 4;
	if (average < 0||average > 100) {
		puts("Invalid grades");
		return 1;
	}
	switch ((int)average / 10) {
		case 10:
		case 9: letter = 'A'; break;
		case 8: letter = 'B'; break;
		case 7: letter = (average >= 75) ? 'C' : 'F'; break;
		default: letter = 'F';
	}
	printf("Average: %.2f\n", average);
	if (average>=75) {
		puts("Remarks: Passed");
	} else {
		puts("Remarks: Failed");
	}
	printf("\n--- Results ---\n");
	return 0;
}
