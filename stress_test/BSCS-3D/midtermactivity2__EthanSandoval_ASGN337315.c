#include <stdio.h>

int main()
{
    // declare the variable to hold the user's input number
    int original_input_number;

    // declare and initialize the running sum of digits
    int total_digit_sum = 0;

    // ask the user for a positive integer
    printf("Enter a positive integer: ");
    scanf("%d", &original_input_number);

    // create a working copy so we do not destroy the original value
    int working_copy_of_number = original_input_number;

    // keep extracting digits until nothing is left
    while (working_copy_of_number > 0)
    {
        // isolate the last digit using the modulus operator
        int extracted_digit = working_copy_of_number % 10;

        // add the extracted digit to the running total
        total_digit_sum = total_digit_sum + extracted_digit;

        // remove the last digit by integer division
        working_copy_of_number = working_copy_of_number / 10;
    }

    // display the final sum of all digits
    printf("\nSum of digits in %d is %d\n", original_input_number, total_digit_sum);

    return 0;
}
