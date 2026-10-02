/*
 * Name: Daniel Estrada
 * Program: Sort, Search and Median
 */

#include <stdio.h>

void print_array(int numbers[], int size);
void sort_array(int numbers[], int size);
int binary_search(int numbers[], int size, int key);

int main(void)
{
    int numbers[50], size;
    scanf("%d", &size);
    for (int i = 0; i < size; i += 1)
    {
        scanf("%d", &numbers[i]);
    }
    sort_array(numbers, size);
    printf("Sorted: ");
    print_array(numbers, size);
    // median of the sorted list
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
    scanf("%d", &key);
    int index = binary_search(numbers, size, key);
    printf(index>=0 ? "Found at %d\n" : "Not found\n", index);

    printf("Smallest: %d, Largest: %d\n", numbers[0], numbers[size - 1]);

    int total = 0;
    for (int i=0; i < size; i += 1)
    {
        total += numbers[i];
    }
    printf("Sum: %d, Average: %.2f\n", total, (float)total / size);

    int duplicates = 0;
    for (int i = 1; i < size; i += 1)
    {
        if (numbers[i] == numbers[i - 1])
        {
            duplicates += 1;
        }
    }
    printf("\n--- Results ---\n");
    printf("Duplicates: %d\n", duplicates);
    return 0;
}

void print_array(int numbers[], int size)
{
    for (int i = 0; i < size; i += 1)
    {
        printf("%d ", numbers[i]);
    }
    puts("");
}

void sort_array(int numbers[], int size)
{
    for (int i = 0; i < size - 1; i += 1)
    {
        for (int j = 0; j < size - i - 1; j+=1)
        {
            if (numbers[j] > numbers[j + 1])
            {
                int temp=numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
            }
        }
    }
}

int binary_search(int numbers[], int size, int key)
{
    for (int i = 0; i < size && numbers[i] <= key; i += 1)
    {
        if (numbers[i] == key)
        {
            return i;
        }
    }
    return -1;
}
