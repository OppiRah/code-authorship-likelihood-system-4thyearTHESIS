/*
 * Name: Denise De Guzman
 * Program: Student Records
 */

#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

int top_student(struct Student students[], int size) {
    int best = 0;
    for (int i = 1; i < size; i++) {
        if (students[i].grade > students[best].grade) {
            best = i;
        }
    }
    return best;
}

float class_average(struct Student students[], int size) {
    float total = 0;
    for (struct Student *ptr = students; ptr < students + size; ptr++) {
        total = total + ptr->grade;
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
    for (int i = 0; i < size; i++) {
        printf("Name of student %d: ", i + 1);
        scanf("%49s", students[i].name);
        printf("Grade: ");
        scanf("%f", &students[i].grade);
    }
    for (int i = 0; i < size; i++) {
        if (students[i].grade >= 75) {
            passed++;
        }
    }
    printf("Class average: %.2f\n", class_average(students, size));
    int best = top_student(students, size);
    printf("Top student: %s (%.2f)\n", students[best].name, students[best].grade);
    printf("Passed: %d, Failed: %d\n", passed, size-passed);
    int worst=0;
    for (int i = 1; i < size; i++) {
        if (students[i].grade < students[worst].grade) {
            worst = i;
        }
    }
    printf("Lowest: %s (%.2f)\n", students[worst].name, students[worst].grade);

    for (int i = 0; i < size; i++) {
        printf("%s: %s\n", students[i].name, students[i].grade >= 75 ? "Passed" : "Failed");
    }
    printf("\n%-20s %s\n", "NAME", "GRADE");
    for (int i = 0; i < size; i++) {
        printf("%-20s %6.2f\n", students[i].name, students[i].grade);
    }
    return 0;
}
