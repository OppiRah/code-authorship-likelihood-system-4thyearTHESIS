/*
 * Name: Bryan Castro
 * Program: Primes in a Range
 */

#include <stdio.h>

int is_prime(int num)
{
    if (num < 2)
    {
        return 0;
    }
    for (int i = 2; i < num; ++i)
    {
        if (num%i == 0)
        {
            return 0;
        }
    }
    return 1;
}

int list_primes(int start, int stop)
{
    int prime_count = 0;
    for (int i = start; i <= stop; ++i)
    {
        if (is_prime(i))
        {
            printf("%d ", i);
            ++prime_count;
        }
    }
    printf("\n");
    return prime_count;
}

int main(void)
{
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    if (start>stop)
    {
        int temp = start;
        start = stop;
        stop = temp;
    }
    int prime_count = list_primes(start, stop);
    printf("%d primes found\n", prime_count);

    int twins = 0;
    for (int i = start; i + 2 <= stop; ++i)
    {
        if (is_prime(i) && is_prime(i + 2))
        {
            ++twins;
        }
    }
    printf("Twin prime pairs: %d\n", twins);

    printf("Numbers checked: %d\n", stop - start + 1);
    return 0;
}
