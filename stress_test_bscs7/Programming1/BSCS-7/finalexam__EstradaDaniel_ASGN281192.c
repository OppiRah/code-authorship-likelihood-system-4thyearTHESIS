/*
 * Name: Daniel Estrada
 * Program: Array Statistics
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

    int maximum=numbers[0], minimum = numbers[0], sum = 0;
    int i;
    // maximum and minimum
    i = 0;
    while (i < size)
    {
        if (numbers[i] > maximum)
        {
            maximum = numbers[i];
        }
        else if (numbers[i] < minimum)
        {
            minimum = numbers[i];
        }
        i += 1;
    }
    i = 0;
    while (i < size)
    {
        sum = sum+numbers[i];
        i += 1;
    }
    float average = (float)sum / size;
    int above = 0;
    for (int i = 0; i < size; i += 1)
    {
        if (numbers[i] > average)
        {
            above += 1;
        }
    }

    printf("Max: %d\n", maximum);
    printf("Min: %d\n", minimum);
    printf("Average: %.2f\n", average);
    printf("Above average: %d\n", above);
    return 0;
}
