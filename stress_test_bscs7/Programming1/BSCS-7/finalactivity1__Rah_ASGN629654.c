#include <stdio.h>
#include <string.h>

// Activity 1: Basic student struct

struct Student {
    char name[50];
    int age;
    double gpa;
};

int main() {
    struct Student currentStudent;

    printf("Enter student name: ");
    scanf("%s", currentStudent.name);

    printf("Enter student age: ");
    scanf("%d", &currentStudent.age);

    printf("Enter student GPA: ");
    scanf("%lf", &currentStudent.gpa);

    printf("\n--- Student Record ---\n");
    printf("Name: %s\n", currentStudent.name);
    printf("Age: %d\n", currentStudent.age);
    printf("GPA: %.2lf\n", currentStudent.gpa);

    return 0;
}
