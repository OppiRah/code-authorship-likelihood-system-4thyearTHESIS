/*
 * Name: Bryan Castro
 * Program: Array Statistics
 */

#include <stdio.h>

int main(void)
{
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i < size; ++i)
    {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }

    int maximum = numbers[0], minimum = numbers[0], sum = 0;
    int i;
    i = 0;
    while (i < size)
    {
        if (numbers[i] > maximum)
        {
            maximum = numbers[i];
        }
        else if (numbers[i]<minimum)
        {
            minimum=numbers[i];
        }
        ++i;
    }
    // total
    i = 0;
    while (i < size)
    {
        sum += numbers[i];
        ++i;
    }
    float average = (float)sum / size;

    printf("Max: %d\nMin: %d\nAverage: %.2f\n", maximum, minimum, average);
    return 0;
}
