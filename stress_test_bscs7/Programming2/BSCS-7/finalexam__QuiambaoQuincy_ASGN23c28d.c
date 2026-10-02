/*
 * Name: Quincy Quiambao
 * Program: Student Records
 */

#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

float class_average(struct Student students[], int size)
{
    float total = 0;
    for (struct Student *ptr = students; ptr < students + size; ptr += 1)
    {
        total += ptr->grade;
    }
    return total / size;
}

int top_student(struct Student students[], int size)
{
    int best = 0;
    for (int i = 1; i < size; i += 1)
    {
        if (students[i].grade > students[best].grade)
        {
            best = i;
        }
    }
    return best;
}

int main(void)
{
    struct Student students[30];
    int size, passed = 0;
    printf("Number of students: ");
    scanf("%d", &size);
    for (int i = 0; i < size; i += 1)
    {
        printf("Name of student %d: ", i + 1);
        scanf("%49s", students[i].name);
        printf("Grade: ");
        scanf("%f", &students[i].grade);
        if (students[i].grade >= 75)
        {
            passed += 1;
        }
    }
    printf("Class average: %.2f\n", class_average(students, size));
    int best = top_student(students, size);
    printf("Top student: %s (%.2f)\n", students[best].name, students[best].grade);
    printf("Passed: %d, Failed: %d\n", passed, size - passed);

    int worst = 0;
    for (int i = 1; i < size; i += 1)
    {
        if (students[i].grade < students[worst].grade)
        {
            worst = i;
        }
    }
    printf("Lowest: %s (%.2f)\n", students[worst].name, students[worst].grade);
    return 0;
}
