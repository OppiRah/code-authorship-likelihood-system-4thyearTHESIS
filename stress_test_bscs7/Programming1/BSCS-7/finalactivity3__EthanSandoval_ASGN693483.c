#include <stdio.h>

int main()
{
    // declare a file pointer that will reference the output file
    FILE *output_file_pointer;

    // declare buffers to hold the student information to be saved
    char student_name_to_save[50];
    int student_age_to_save;

    // attempt to open the file in write mode
    output_file_pointer = fopen("student_data.txt", "w");

    // check whether the file was opened successfully
    if (output_file_pointer == NULL)
    {
        printf("Error: Could not open file for writing.\n");
        return 1;
    }

    // ask the user for the student's name
    printf("Enter student name: ");
    scanf("%s", student_name_to_save);

    // ask the user for the student's age
    printf("Enter student age: ");
    scanf("%d", &student_age_to_save);

    // write the name and age to the file
    fprintf(output_file_pointer, "Name: %s\n", student_name_to_save);
    fprintf(output_file_pointer, "Age: %d\n", student_age_to_save);

    // close the file now that we are finished writing to it
    fclose(output_file_pointer);

    // inform the user that the write operation succeeded
    printf("\nData successfully written to student_data.txt\n");

    return 0;
}
