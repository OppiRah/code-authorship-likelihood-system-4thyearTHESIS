/*
 * Name: Francis Galang
 * Program: Array Statistics
 */

#include <stdio.h>

int main(void)
{
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    if (size <= 0 || size > 50)
    {
        printf("Invalid size\n");
        return 1;
    }
    for (int i = 0; i < size; i++)
    {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    int maximum = numbers[0], minimum = numbers[0], sum = 0;
    int i;
    i = 0;
    while (i < size)
    {
        sum = sum + numbers[i];
        if (numbers[i] > maximum)
        {
            maximum = numbers[i];
        }
        if (numbers[i] < minimum)
        {
            minimum = numbers[i];
        }
        i++;
    }
    float average = sum * 1.0 / size;
    int above = 0;
    for (int i = 0; i < size; i++)
    {
        if (numbers[i] > average)
        {
            above++;
        }
    }
    printf("Max: %d\nMin: %d\nAverage: %.2f\nAbove average: %d\n", maximum, minimum, average, above);
}
