#include <stdio.h>

// this function adds two numbers together and returns the result
double add_two_numbers_together(double first_value, double second_value)
{
    return first_value + second_value;
}

// this function subtracts the second number from the first
double subtract_second_from_first(double first_value, double second_value)
{
    return first_value - second_value;
}

// this function multiplies two numbers together and returns the result
double multiply_two_numbers_together(double first_value, double second_value)
{
    return first_value * second_value;
}

int main()
{
    // declare variables to hold the two numbers the user provides
    double first_operand_value;
    double second_operand_value;

    // ask the user for the first number
    printf("Enter the first number: ");
    scanf("%lf", &first_operand_value);

    // ask the user for the second number
    printf("Enter the second number: ");
    scanf("%lf", &second_operand_value);

    // call each function and display its result
    printf("\nSum: %.2lf\n", add_two_numbers_together(first_operand_value, second_operand_value));
    printf("Difference: %.2lf\n", subtract_second_from_first(first_operand_value, second_operand_value));
    printf("Product: %.2lf\n", multiply_two_numbers_together(first_operand_value, second_operand_value));

    return 0;
}
