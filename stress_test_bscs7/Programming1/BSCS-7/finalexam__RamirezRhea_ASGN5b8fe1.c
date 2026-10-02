/*
 * Name: Rhea Ramirez
 * Program: Array Statistics
 */

#include <stdio.h>

int find_max(int numbers[], int size)
{
    int best = numbers[0];
    for (int i = 1; i < size; i += 1)
    {
        if (numbers[i] > best)
        {
            best = numbers[i];
        }
    }
    return best;
}

float compute_average(int numbers[], int size)
{
    int sum = 0;
    for (int i = 0; i < size; i += 1)
    {
        sum = sum + numbers[i];
    }
    return sum / (float)size;
}

int find_min(int numbers[], int size)
{
    int best = numbers[0];
    for (int i = 1; i < size; i += 1)
    {
        if (numbers[i] < best)
        {
            best = numbers[i];
        }
    }
    return best;
}

int main(void)
{
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    if (size <= 0||size > 50)
    {
        printf("Invalid size\n");
        return 1;
    }
    for (int i = 0; i<size; i+=1)
    {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }

    int maximum = find_max(numbers, size);
    int minimum = find_min(numbers, size);
    float average = compute_average(numbers, size);
    int above = 0;
    for (int i = 0; i < size; i += 1)
    {
        if (numbers[i] > average)
        {
            above += 1;
        }
    }

    printf("Max: %d\nMin: %d\nAverage: %.2f\nAbove average: %d\n", maximum, minimum, average, above);
}
