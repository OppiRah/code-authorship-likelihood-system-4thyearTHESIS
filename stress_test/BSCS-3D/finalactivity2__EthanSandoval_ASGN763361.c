#include <stdio.h>

// this function swaps the values pointed to by the two pointers
void swap_two_integer_values(int *pointer_to_first, int *pointer_to_second)
{
    // temporarily store the first value before overwriting it
    int temporary_storage_value = *pointer_to_first;

    // copy the second value into the first location
    *pointer_to_first = *pointer_to_second;

    // copy the saved first value into the second location
    *pointer_to_second = temporary_storage_value;
}

int main()
{
    // declare the two values that will be swapped
    int first_number_value;
    int second_number_value;

    // ask the user for the first value
    printf("Enter the first value: ");
    scanf("%d", &first_number_value);

    // ask the user for the second value
    printf("Enter the second value: ");
    scanf("%d", &second_number_value);

    // display the values before the swap occurs
    printf("\nBefore swap: %d, %d\n", first_number_value, second_number_value);

    // call the swap function, passing addresses of both variables
    swap_two_integer_values(&first_number_value, &second_number_value);

    // display the values after the swap has occurred
    printf("After swap: %d, %d\n", first_number_value, second_number_value);

    return 0;
}
