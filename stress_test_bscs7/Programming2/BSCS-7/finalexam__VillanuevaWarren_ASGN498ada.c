/*
 * Name: Warren Villanueva
 * Program: Student Records
 */

#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

float class_average(struct Student students[], int size) {
    float total = 0;
    for (int i=0; i < size; i += 1) {
        total=total + students[i].grade;
    }
    return total / size;
}

int main() {
    struct Student students[30];
    int size, passed = 0;
    printf("Number of students: ");
    scanf("%d", &size);
    if (size <= 0 || size > 50) {
        printf("Invalid size\n");
        return 1;
    }
    for (int i = 0; i < size; i += 1) {
        printf("Name of student %d: ", i + 1);
        scanf("%49s", students[i].name);
        printf("Grade: ");
        scanf("%f", &students[i].grade);
        if (students[i].grade >= 75) {
            passed += 1;
        }
    }
    printf("Class average: %.2f\n", class_average(students, size));
    int best = 0;
    for (int i = 1; i < size; i += 1) {
        if (students[i].grade > students[best].grade) {
            best = i;
        }
    }
    printf("Top student: %s (%.2f)\n", students[best].name, students[best].grade);
    printf("Passed: %d, Failed: %d\n", passed, size - passed);

    int honors = 0;
    for (int i = 0; i < size; i+=1) {
        if (students[i].grade >= 90) {
            honors += 1;
        }
    }
    printf("\n--- Results ---\n");
    printf("With honors: %d\n", honors);
}
