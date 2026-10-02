/*
 * Name: Daniel Estrada
 * Program: Array Maximum
 */

#include <stdio.h>

int main(void)
{
    int numbers[50], size;
    scanf("%d", &size);
    for (int i = 0; i < size; i += 1)
    {
        scanf("%d", &numbers[i]);
    }
    int maximum = numbers[0], position = 0;
    // look for the largest element
    for (int i = 1; i < size; i += 1)
    {
        if (numbers[i] > maximum)
        {
            maximum = numbers[i];
            // remember where
            position = i;
        }
    }
    printf("Max = %d at index %d\n", maximum, position);
    return 0;
}
