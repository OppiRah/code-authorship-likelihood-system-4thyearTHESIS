/*
 * Name: Quincy Quiambao
 * Program: Primes in a Range
 */

#include <stdio.h>

int is_prime(int num)
{
    if (num < 2)
    {
        return 0;
    }
    for (int i = 2; i * i <= num; i += 1)
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
    for (int i = start; i <= stop; i+=1)
    {
        if (is_prime(i))
        {
            printf("%d ", i);
            prime_count += 1;
        }
    }
    puts("");
    return prime_count;
}

int main(void)
{
    int start, stop;
    printf("Enter range (start end): ");
    scanf("%d %d", &start, &stop);
    int prime_count = list_primes(start, stop);
    printf("%d primes found\n", prime_count);

    printf("Numbers checked: %d\n", stop - start + 1);

    int largest = -1;
    for (int i = stop; i >= start; i--)
    {
        if (is_prime(i))
        {
            largest = i;
            break;
        }
    }
    printf("\n--- Results ---\n");
    printf("Largest prime: %d\n", largest);
    return 0;
}
