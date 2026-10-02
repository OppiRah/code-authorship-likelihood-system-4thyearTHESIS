// Isabel Jimenez - Average of Three Grades

#include <stdio.h>

int main() {
	float grade1, grade2, grade3, average;

	printf("Enter three grades: ");
	scanf("%f %f %f", &grade1, &grade2, &grade3);
	// average of three
	average = (grade1 + grade2 + grade3) / 3;
	printf("Average: %.2f\n", average);
	if (average >= 75) {
		puts("Passed");
	}
	else {
		puts("Failed");
	}
	return 0;
}
