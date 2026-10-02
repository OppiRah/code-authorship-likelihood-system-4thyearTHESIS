#include <stdio.h>

int main(void)
{
	float grade1, grade2, grade3, average;

	printf("Enter three grades: ");
	scanf("%f %f %f", &grade1, &grade2, &grade3);
	// average of three
	average = (grade1+grade2 + grade3) / 3;
	printf("Average: %.2f\n", average);
	if (average >= 75) {
		printf("Passed\n");
	} else {
		printf("Failed\n");
	}
	return 0;
}
