/*
 * Name: Daniel Estrada
 * Program: Student Records
 */

#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

float class_average(struct Student students[], int size);

int main(void)
{
    struct Student students[30];
    int size, passed = 0;
    scanf("%d", &size);
    if (size <= 0 || size > 50)
    {
        puts("Invalid size");
        return 1;
    }
    for (int i = 0; i < size; i += 1)
    {
        scanf("%49s", students[i].name);
        scanf("%f", &students[i].grade);
        if (students[i].grade >= 75)
        {
            passed += 1;
        }
    }
    printf("Class average: %.2f\n", class_average(students, size));
    int best = 0;
    for (int i = 1; i < size; i += 1)
    {
        if (students[i].grade > students[best].grade)
        {
            best = i;
        }
    }
    printf("Top student: %s (%.2f)\n", students[best].name, students[best].grade);
    printf("Passed: %d, Failed: %d\n", passed, size - passed);

    int honors = 0;
    for (int i = 0; i < size; i += 1)
    {
        if (students[i].grade >= 90)
        {
            honors += 1;
        }
    }
    printf("With honors: %d\n", honors);
    return 0;
}

float class_average(struct Student students[], int size)
{
    float total = 0;
    for (struct Student *ptr = students; ptr < students + size; ptr += 1)
    {
        total += ptr->grade;
    }
    return total / size;
}
