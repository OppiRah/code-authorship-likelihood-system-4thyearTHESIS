/*
 * Name: Ulysses Umali
 * Program: Student Records
 */

#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

int topStudent(struct Student students[], int size) {
    int best = 0;
    for (int i = 1; i < size; i++) {
        if (students[i].grade > students[best].grade) {
            best = i;
        }
    }
    return best;
}

float classAverage(struct Student students[], int size) {
    float total = 0;
    for (int i = 0; i < size; i++) {
        total = total + students[i].grade;
    }
    return total / size;
}

int main(void) {
    struct Student students[30];
    int size, passed = 0;
    printf("Number of students: ");
    scanf("%d", &size);
    for (int i = 0; i<size; i++) {
        printf("Name of student %d: ", i + 1);
        scanf("%49s", students[i].name);
        printf("Grade: ");
        scanf("%f", &students[i].grade);
        if (students[i].grade >= 75) {
            passed++;
        }
    }
    printf("Class average: %.2f\n", classAverage(students, size));
    int best = topStudent(students, size);
    printf("Top student: %s (%.2f)\n", students[best].name, students[best].grade);
    printf("Passed: %d, Failed: %d\n", passed, size - passed);

    int honors = 0;
    for (int i = 0; i < size; i++) {
        if (students[i].grade >= 90) {
            honors++;
        }
    }
    printf("With honors: %d\n", honors);
    int worst = 0;
    for (int i = 1; i < size; i++) {
        if (students[i].grade < students[worst].grade) {
            worst = i;
        }
    }
    printf("\n--- Results ---\n");
    printf("Lowest: %s (%.2f)\n", students[worst].name, students[worst].grade);
    return 0;
}
