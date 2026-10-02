#include <stdio.h>

int main()
{
    // declare the variable that will hold the base number
    int base_number_for_table;

    // ask the user which number to generate a table for
    printf("Enter a number to generate its multiplication table: ");
    scanf("%d", &base_number_for_table);

    printf("\nMultiplication table for %d:\n", base_number_for_table);

    // set up a multiplier starting from 1
    int current_multiplier = 1;

    // loop through multipliers 1 through 10
    while (current_multiplier <= 10)
    {
        // calculate and print this line of the table
        printf("%d x %d = %d\n", base_number_for_table, current_multiplier,
               base_number_for_table * current_multiplier);

        // move to the next multiplier
        current_multiplier = current_multiplier + 1;
    }

    return 0;
}
