#include <stdio.h>

int main()
{
    // declare an array large enough to hold the values to sort
    int array_to_be_sorted[20];

    // declare the variable that holds the array size
    int total_element_count;

    // ask the user how many numbers they will be sorting
    printf("Enter how many numbers to sort: ");
    scanf("%d", &total_element_count);

    // set up a loop counter for reading the input values
    int fill_index = 0;

    // read each number into the array one at a time
    while (fill_index < total_element_count)
    {
        printf("Enter number %d: ", fill_index + 1);
        scanf("%d", &array_to_be_sorted[fill_index]);
        fill_index = fill_index + 1;
    }

    // set up the outer pass counter for bubble sort
    int current_pass = 0;

    // repeat passes until the array is fully sorted
    while (current_pass < total_element_count - 1)
    {
        // set up the inner comparison counter for this pass
        int compare_position = 0;

        // compare each adjacent pair within this pass
        while (compare_position < total_element_count - current_pass - 1)
        {
            // check if the two adjacent elements are out of order
            if (array_to_be_sorted[compare_position] > array_to_be_sorted[compare_position + 1])
            {
                // swap the two elements using a temporary variable
                int temporary_holder = array_to_be_sorted[compare_position];
                array_to_be_sorted[compare_position] = array_to_be_sorted[compare_position + 1];
                array_to_be_sorted[compare_position + 1] = temporary_holder;
            }

            compare_position = compare_position + 1;
        }

        current_pass = current_pass + 1;
    }

    // display the fully sorted array to the user
    printf("\nSorted array: ");
    int print_index = 0;
    while (print_index < total_element_count)
    {
        printf("%d ", array_to_be_sorted[print_index]);
        print_index = print_index + 1;
    }
    printf("\n");

    return 0;
}
