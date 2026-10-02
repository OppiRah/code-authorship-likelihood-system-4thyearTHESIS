/*
 * Name: Daniel Estrada
 * Program: Linear Search
 */

#include <stdio.h>

int linear_search(int numbers[], int size, int key)
{
    for (int i = 0; i < size; i+=1)
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
    int numbers[]={5, 17, 3, 42, 8, 29};
    int key;
    scanf("%d", &key);
    int index = linear_search(numbers, 6, key);
    if (index >= 0)
    {
        printf("Found at %d\n", index);
    }
    else
    {
        puts("Not found");
    }
    return 0;
}
