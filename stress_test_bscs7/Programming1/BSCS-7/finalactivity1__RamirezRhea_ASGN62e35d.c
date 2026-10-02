/*
 * Name: Rhea Ramirez
 * Program: Array Sum and Average
 */

#include <stdio.h>

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
    int sum=0;

    for (int i = 0; i < size; i += 1)
    {
        sum = sum + numbers[i];
    }
    printf("Sum = %d\n", sum);
    printf("Average = %.2f\n", sum / (float)size);
}
