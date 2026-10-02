// Isabel Jimenez - Student Struct

#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

void printStudent(struct Student student) {
	printf("Name: %s\n", student.name);
	printf("Grade: %.2f\n", student.grade);
}

int main() {
	struct Student student;
	printf("Name: ");
	scanf("%49s", student.name);
	printf("Grade: ");
	scanf("%f", &student.grade);
	printStudent(student);
	return 0;
}
