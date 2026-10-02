// Isabel Jimenez - Student Records

#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

float classAverage(struct Student students[], int size) {
	float total=0;
	for (int i = 0; i < size; ++i) {
		total = total + students[i].grade;
	}
	return total / size;
}

int main() {
	struct Student students[30];
	int size, passed=0;
	printf("Number of students: ");
	scanf("%d", &size);
	if (size <= 0 || size > 50) {
		puts("Invalid size");
		return 1;
	}
	for (int i = 0; i < size; ++i) {
		printf("Name of student %d: ", i + 1);
		scanf("%49s", students[i].name);
		printf("Grade: ");
		scanf("%f", &students[i].grade);
	}
	for (int i = 0; i < size; ++i) {
		if (students[i].grade >= 75) {
			++passed;
		}
	}
	printf("Class average: %.2f\n", classAverage(students, size));
	int best = 0;
	for (int i = 1; i < size; ++i) {
		if (students[i].grade > students[best].grade) {
			best = i;
		}
	}
	printf("Top student: %s (%.2f)\n", students[best].name, students[best].grade);
	printf("Passed: %d, Failed: %d\n", passed, size - passed);
	for (int i = 0; i < size; ++i) {
		printf("%s: %s\n", students[i].name, students[i].grade >= 75 ? "Passed" : "Failed");
	}
	int worst = 0;
	for (int i=1; i < size; ++i) {
		if (students[i].grade < students[worst].grade) {
			worst = i;
		}
	}
	printf("Lowest: %s (%.2f)\n", students[worst].name, students[worst].grade);

	printf("\n%-20s %s\n", "NAME", "GRADE");
	for (int i = 0; i<size; ++i) {
		printf("%-20s %6.2f\n", students[i].name, students[i].grade);
	}
	printf("\n--- Results ---\n");
	return 0;
}
