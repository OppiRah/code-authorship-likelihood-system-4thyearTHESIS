/*
 * Name: Bryan Castro
 * Program: Sort, Search and Median
 */

#include <stdio.h>

int binary_search(int numbers[], int size, int key)
{
    int low = 0, high = size - 1;
    while (low <= high)
    {
        int mid = (low + high) / 2;
        if (numbers[mid] == key)
        {
            return mid;
        }
        else if (numbers[mid]<key)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }
    return -1;
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
                numbers[j] = numbers[j+1];
                numbers[j + 1] = temp;
                swapped = 1;
            }
        }
        if (!swapped)
        {
            break;
        }
    }
}

void print_array(int numbers[], int size)
{
    for (int i = 0; i < size; ++i)
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
    for (int i = 0; i < size; ++i)
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
    int index=binary_search(numbers, size, key);
    printf(index >= 0 ? "Found at %d\n" : "Not found\n", index);

    int total = 0;
    for (int i = 0; i < size; ++i)
    {
        total += numbers[i];
    }
    printf("Sum: %d, Average: %.2f\n", total, (float)total / size);

    int duplicates = 0;
    for (int i = 1; i < size; ++i)
    {
        if (numbers[i] == numbers[i-1])
        {
            ++duplicates;
        }
    }
    printf("\n--- Results ---\n");
    printf("Duplicates: %d\n", duplicates);
    return 0;
}
