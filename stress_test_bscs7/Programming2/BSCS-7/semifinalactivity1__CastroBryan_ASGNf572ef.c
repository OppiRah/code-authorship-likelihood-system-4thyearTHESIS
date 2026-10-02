/*
 * Name: Bryan Castro
 * Program: Bubble Sort
 */

#include <stdio.h>

void print_array(int numbers[], int size)
{
    for (int i = 0; i < size; ++i)
    {
        printf("%d ", numbers[i]);
    }
    printf("\n");
}

void sort_array(int numbers[], int size)
{
    int swapped;
    for (int i = 0; i < size - 1; ++i)
    {
        swapped = 0;
        for (int j = 0; j < size - i - 1; ++j)
        {
            if (numbers[j] > numbers[j + 1])
            {
                int temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
                swapped = 1;
            }
        }
        if (!swapped)
        {
            // already sorted
            break;
        }
    }
}

int main(void)
{
    int numbers[] = {64, 25, 12, 22, 11, 90, 38};
    int size = 7;
    printf("Before: ");
    print_array(numbers, size);
    sort_array(numbers, size);
    printf("After: ");
    print_array(numbers, size);
    return 0;
}
