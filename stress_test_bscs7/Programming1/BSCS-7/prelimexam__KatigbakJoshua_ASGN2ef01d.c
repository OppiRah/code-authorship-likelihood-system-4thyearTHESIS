#include <stdio.h>

int main(void)
{
	float grade1, grade2, grade3, average;
	char letter;

	// input
	printf("Enter grade: ");
	scanf("%f", &grade1);
	printf("Enter grade: ");
	scanf("%f", &grade2);
	printf("Enter grade: ");
	scanf("%f", &grade3);
	average = (grade1 + grade2 + grade3) / 3;
	if (average < 0 || average > 100) {
		printf("Invalid grades\n");
		return 1;
	}
	// letter grade
	if (average >= 90) {
		letter = 'A';
	} else if (average >= 80) {
		letter = 'B';
	} else if (average >= 75) {
		letter = 'C';
	} else {
		letter = 'F';
	}

	printf("\n--- Results ---\n");
	printf("Average: %.2f\n", average);
	printf("Grade: %c\n", letter);
	printf("Remarks: %s\n", average >= 75 ? "Passed" : "Failed");
	return 0;
}
