// Isabel Jimenez - Grade Computation

#include <stdio.h>

int main() {
	float grades[3], total=0, average;
	char letter;

	for (int i = 0; i < 3; ++i) {
		printf("Grade %d: ", i+1);
		scanf("%f", &grades[i]);
		total += grades[i];
	}
	// compute the average
	average=total / 3;
	if (average >= 90) {
		letter = 'A';
	}
	else if (average >= 80) {
		letter='B';
	}
	else if (average>=75) {
		letter = 'C';
	}
	else {
		letter = 'F';
	}
	printf("\n--- Results ---\n");
	printf("Average: %.2f\n", average);
	printf("Grade: %c\n", letter);
	printf("Remarks: %s\n", average >= 75 ? "Passed" : "Failed");
	return 0;
}
