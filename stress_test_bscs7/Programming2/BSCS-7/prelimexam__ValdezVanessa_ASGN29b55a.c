/*
 * Name: Vanessa Valdez
 * Program: Primes in a Range
 */

#include <stdio.h>

int is_prime(int num)
{
    if (num < 2)
    {
        return 0;
    }
    for (int i = 2; i < num; i++)
    {
        if (num % i == 0)
        {
            // found a divisor
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    if (start > stop)
    {
        int temp = start;
        start = stop;
        stop = temp;
    }
    int prime_count = 0, prime_sum = 0;
    int numbers[500];
    for (int i = start; i <= stop; i++)
    {
        if (is_prime(i))
        {
            numbers[prime_count] = i;
            prime_count++;
            prime_sum += i;
        }
    }
    for (int i = 0; i < prime_count; i++)
    {
        printf("%d ", numbers[i]);
    }
    printf("\nCount: %d, Sum: %d\n", prime_count, prime_sum);

    int composites = 0;
    for (int i = start; i <= stop; i++)
    {
        if (i > 1 && !is_prime(i))
        {
            composites = composites + 1;
        }
    }
    printf("\n--- Results ---\n");
    printf("Composite numbers: %d\n", composites);
    return 0;
}
