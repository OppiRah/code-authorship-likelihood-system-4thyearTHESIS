#include <stdio.h>

int main()
{
    // declare an array large enough to hold the input numbers
    int array_of_numbers[20];

    // declare the variable that holds how many numbers were entered
    int total_number_count;

    // ask the user how many numbers they will input
    printf("Enter how many numbers to input: ");
    scanf("%d", &total_number_count);

    // set up a loop counter for reading the input values
    int input_index = 0;

    // read each number into the array one at a time
    while (input_index < total_number_count)
    {
        printf("Enter number %d: ", input_index + 1);
        scanf("%d", &array_of_numbers[input_index]);
        input_index = input_index + 1;
    }

    // assume the first element is both the max and min initially
    int highest_value_found = array_of_numbers[0];
    int lowest_value_found = array_of_numbers[0];

    // reset the index for the comparison loop
    int compare_index = 1;

    // scan the rest of the array to find the true max and min
    while (compare_index < total_number_count)
    {
        // check if the current value is higher than our current max
        if (array_of_numbers[compare_index] > highest_value_found)
        {
            highest_value_found = array_of_numbers[compare_index];
        }

        // check if the current value is lower than our current min
        if (array_of_numbers[compare_index] < lowest_value_found)
        {
            lowest_value_found = array_of_numbers[compare_index];
        }

        compare_index = compare_index + 1;
    }

    // display the final highest and lowest values
    printf("\nMaximum value: %d\n", highest_value_found);
    printf("Minimum value: %d\n", lowest_value_found);

    return 0;
}
