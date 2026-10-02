/*
 * Name: Daniel Estrada
 * Program: Array Sum and Average
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
    int sum = 0;

    // add all elements
    for (int i = 0; i < size; i += 1)
    {
        sum+=numbers[i];
    }
    printf("Sum = %d\n", sum);
    printf("Average = %.2f\n", (float)sum / size);
    return 0;
}
