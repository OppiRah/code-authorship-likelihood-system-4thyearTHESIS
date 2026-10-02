#include <stdio.h>

struct student {
    char name[50];
    int age;
    double gpa;
};

int main() {
    struct student s;
    printf("Enter name: ");
    scanf("%s", s.name);
    printf("Enter age: ");
    scanf("%d", &s.age);
    printf("Enter GPA: ");
    scanf("%lf", &s.gpa);
    printf("\nName: %s\n", s.name);
    printf("Age: %d\n", s.age);
    printf("GPA: %.2lf\n", s.gpa);
    return 0;
}
