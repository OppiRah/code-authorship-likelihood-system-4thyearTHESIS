// Aaron Aquino - Student Records

#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

float classAverage(struct Student students[], int size);

int main() {
    struct Student students[30];
    int size, passed = 0;
    scanf("%d", &size);
    for (int i = 0; i < size; ++i) {
        scanf("%49s", students[i].name);
        scanf("%f", &students[i].grade);
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
    printf("\n%-20s %s\n", "NAME", "GRADE");
    for (int i = 0; i < size; ++i) {
        printf("%-20s %6.2f\n", students[i].name, students[i].grade);
    }

    for (int i = 0; i < size; ++i) {
        printf("%s: %s\n", students[i].name, students[i].grade >= 75 ? "Passed" : "Failed");
    }
    int worst = 0;
    for (int i = 1; i<size; ++i) {
        if (students[i].grade < students[worst].grade) {
            worst = i;
        }
    }
    printf("Lowest: %s (%.2f)\n", students[worst].name, students[worst].grade);
    return 0;
}

float classAverage(struct Student students[], int size) {
    float total = 0;
    for (int i = 0; i < size; ++i) {
        total = total + students[i].grade;
    }
    return total / size;
}
