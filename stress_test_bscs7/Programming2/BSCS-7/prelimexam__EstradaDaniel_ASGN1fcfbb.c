/*
 * Name: Daniel Estrada
 * Program: Primes in a Range
 */

#include <stdio.h>

int is_prime(int num)
{
    if (num < 2)
    {
        return 0;
    }
    for (int i = 2; i <= num / 2; i += 1)
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
    scanf("%d %d", &start, &stop);
    int prime_count = 0, prime_sum = 0;
    int numbers[500];
    for (int i=start; i <= stop; i += 1)
    {
        if (is_prime(i))
        {
            numbers[prime_count] = i;
            prime_count += 1;
            prime_sum += i;
        }
    }
    for (int i = 0; i < prime_count; i += 1)
    {
        printf("%d ", numbers[i]);
    }
    printf("\nCount: %d, Sum: %d\n", prime_count, prime_sum);

    int largest = -1;
    for (int i = stop; i >= start; i--)
    {
        if (is_prime(i))
        {
            largest = i;
            break;
        }
    }
    printf("Largest prime: %d\n", largest);

    int twins = 0;
    for (int i = start; i + 2 <= stop; i += 1)
    {
        if (is_prime(i) && is_prime(i + 2))
        {
            twins += 1;
        }
    }
    printf("Twin prime pairs: %d\n", twins);
    return 0;
}
