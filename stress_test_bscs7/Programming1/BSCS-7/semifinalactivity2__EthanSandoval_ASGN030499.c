#include <stdio.h>

// this function checks whether the given number is a prime number
int check_if_number_is_prime(int number_to_check)
{
    // numbers less than 2 are not considered prime
    if (number_to_check < 2)
    {
        return 0;
    }

    // set up a divisor starting from 2
    int current_divisor = 2;

    // test divisors up to the square root of the number
    while (current_divisor * current_divisor <= number_to_check)
    {
        // if the number divides evenly, it is not prime
        if (number_to_check % current_divisor == 0)
        {
            return 0;
        }

        current_divisor = current_divisor + 1;
    }

    // no divisors were found, so the number must be prime
    return 1;
}

int main()
{
    // declare the variable to hold the user's input number
    int number_entered_by_user;

    // ask the user for a number to check
    printf("Enter a number to check: ");
    scanf("%d", &number_entered_by_user);

    // call the prime-checking function and report the result
    if (check_if_number_is_prime(number_entered_by_user))
    {
        printf("\n%d is a prime number.\n", number_entered_by_user);
    }
    else
    {
        printf("\n%d is not a prime number.\n", number_entered_by_user);
    }

    return 0;
}
