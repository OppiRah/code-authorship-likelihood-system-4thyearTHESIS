/*
 * Name: Daniel Estrada
 * Program: Count Even and Odd
 */

#include <stdio.h>

int main(void)
{
    int numbers[50], size;
    scanf("%d", &size);
    for (int i = 0; i < size; i+=1)
    {
        scanf("%d", &numbers[i]);
    }
    int even_count = 0, odd_count = 0;

    for (int i = 0; i < size; i += 1)
    {
        if (numbers[i] % 2 == 0)
        {
            even_count += 1;
        }
        else
        {
            odd_count += 1;
        }
    }
    printf("Even: %d, Odd: %d\n", even_count, odd_count);
    return 0;
}
