#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

void print_student(struct Student student)
{
	printf("Name: %s\n", student.name);
}

int main(void)
{
	struct Student student;
	printf("Name: ");
	scanf("%49s", student.name);
	scanf("%f", &student.grade);
	print_student(student);
	return 0;
}
