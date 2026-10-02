#include <stdio.h>

// Activity 3: Write data to a file

int main() {
    FILE *outputFile;
    char studentName[50];
    int studentAge;

    outputFile = fopen("student_data.txt", "w");

    if (outputFile == NULL) {
        printf("Error: Could not open file for writing.\n");
        return 1;
    }

    printf("Enter student name: ");
    scanf("%s", studentName);

    printf("Enter student age: ");
    scanf("%d", &studentAge);

    // Write the data to the file
    fprintf(outputFile, "Name: %s\n", studentName);
    fprintf(outputFile, "Age: %d\n", studentAge);

    fclose(outputFile);

    printf("\nData successfully written to student_data.txt\n");

    return 0;
}
