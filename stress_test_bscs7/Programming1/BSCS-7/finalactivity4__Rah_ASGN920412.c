#include <stdio.h>

// Activity 4: Read data from a file

int main() {
    FILE *inputFile;
    char lineBuffer[100];

    inputFile = fopen("student_data.txt", "r");

    if (inputFile == NULL) {
        printf("Error: Could not open file for reading.\n");
        return 1;
    }

    printf("--- Contents of student_data.txt ---\n");

    // Read and print each line until end of file
    while (fgets(lineBuffer, sizeof(lineBuffer), inputFile) != NULL) {
        printf("%s", lineBuffer);
    }

    fclose(inputFile);

    return 0;
}
