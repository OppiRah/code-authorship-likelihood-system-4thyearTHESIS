/*
 * Name: Rhea Ramirez
 * Program: Sort, Search and Median
 */

#include <stdio.h>

int binary_search(int numbers[], int size, int key)
{
    // array is sorted so we can stop early
    for (int i=0; i < size && numbers[i] <= key; i += 1)
    {
        if (numbers[i] == key)
        {
            return i;
        }
    }
    return -1;
}

void sort_array(int numbers[], int size)
{
    for (int i = 0; i < size-1; i += 1)
    {
        int min_index = i;
        for (int j=i + 1; j < size; j += 1)
        {
            if (numbers[j] < numbers[min_index])
            {
                min_index = j;
            }
        }
        int temp = numbers[i];
        numbers[i] = numbers[min_index];
        numbers[min_index] = temp;
    }
}

void print_array(int numbers[], int size)
{
    for (int i = 0; i < size; i += 1)
    {
        printf("%d ", numbers[i]);
    }
    printf("\n");
}

int main(void)
{
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i < size; i += 1)
    {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    printf("You entered: ");
    for (int i = 0; i < size; i += 1)
    {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    sort_array(numbers, size);
    printf("Sorted: ");
    print_array(numbers, size);
    float median;
    if (size % 2 == 0)
    {
        median = (numbers[size / 2 - 1] + numbers[size / 2]) / 2.0;
    }
    else
    {
        median = numbers[size / 2];
    }
    printf("Median: %.1f\n", median);
    int key;
    printf("Search for: ");
    scanf("%d", &key);
    int index = binary_search(numbers, size, key);
    printf(index >= 0 ? "Found at %d\n" : "Not found\n", index);

    printf("Smallest: %d, Largest: %d\n", numbers[0], numbers[size - 1]);

    int duplicates = 0;
    for (int i = 1; i < size; i += 1)
    {
        if (numbers[i] == numbers[i - 1])
        {
            duplicates += 1;
        }
    }
    printf("Duplicates: %d\n", duplicates);
}
