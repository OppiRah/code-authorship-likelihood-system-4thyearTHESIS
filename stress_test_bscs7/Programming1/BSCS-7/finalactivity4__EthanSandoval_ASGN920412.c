#include <stdio.h>

int main()
{
    // declare a file pointer that will reference the input file
    FILE *input_file_pointer;

    // declare a buffer to hold each line as it is read
    char line_reading_buffer[100];

    // attempt to open the file in read mode
    input_file_pointer = fopen("student_data.txt", "r");

    // check whether the file was opened successfully
    if (input_file_pointer == NULL)
    {
        printf("Error: Could not open file for reading.\n");
        return 1;
    }

    printf("--- Contents of student_data.txt ---\n");

    // read and display each line until the end of the file is reached
    while (fgets(line_reading_buffer, sizeof(line_reading_buffer), input_file_pointer) != NULL)
    {
        printf("%s", line_reading_buffer);
    }

    // close the file now that we are finished reading from it
    fclose(input_file_pointer);

    return 0;
}
