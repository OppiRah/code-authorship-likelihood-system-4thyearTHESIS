/*
 * Name: Francis Galang
 * Program: Sort, Search and Median
 */

#include <stdio.h>

void print_array(int numbers[], int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%d ", numbers[i]);
    }
    printf("\n");
}

void sort_array(int numbers[], int size)
{
    int swapped;
    for (int i = 0; i < size - 1; i++)
    {
        swapped = 0;
        for (int j = 0; j < size - i - 1; j++)
        {
            if (numbers[j] > numbers[j + 1])
            {
                int temp=numbers[j];
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

int binary_search(int numbers[], int size, int key)
{
    for (int i = 0; i < size && numbers[i] <= key; i++)
    {
        if (numbers[i] == key)
        {
            return i;
        }
    }
    return -1;
}

int main(void)
{
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i < size; i++)
    {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
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
    printf(index>=0 ? "Found at %d\n" : "Not found\n", index);

    int total = 0;
    for (int i = 0; i < size; i++)
    {
        total = total + numbers[i];
    }
    printf("Sum: %d, Average: %.2f\n", total, total * 1.0 / size);
    int duplicates = 0;
    for (int i = 1; i < size; i++)
    {
        if (numbers[i] == numbers[i - 1])
        {
            duplicates++;
        }
    }
    printf("Duplicates: %d\n", duplicates);
}
