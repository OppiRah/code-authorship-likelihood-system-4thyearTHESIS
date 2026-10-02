#include <stdio.h>

// this structure holds all the information about a single student
struct Student
{
    char full_name[50];
    int age_in_years;
    double grade_point_average;
};

int main()
{
    // declare a single student record to be filled in
    struct Student current_student_record;

    // ask the user for the student's name
    printf("Enter student name: ");
    scanf("%s", current_student_record.full_name);

    // ask the user for the student's age
    printf("Enter student age: ");
    scanf("%d", &current_student_record.age_in_years);

    // ask the user for the student's GPA
    printf("Enter student GPA: ");
    scanf("%lf", &current_student_record.grade_point_average);

    // display the complete student record back to the user
    printf("\n--- Student Record ---\n");
    printf("Name: %s\n", current_student_record.full_name);
    printf("Age: %d\n", current_student_record.age_in_years);
    printf("GPA: %.2lf\n", current_student_record.grade_point_average);

    return 0;
}
