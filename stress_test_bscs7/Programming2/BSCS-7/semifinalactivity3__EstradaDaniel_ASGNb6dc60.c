/*
 * Name: Daniel Estrada
 * Program: Matrix Row Sums
 */

#include <stdio.h>

int main(void)
{
    int matrix[3][3], total = 0;
    for (int i = 0; i < 3; i += 1)
    {
        for (int j = 0; j < 3; j += 1)
        {
            printf("m[%d][%d]: ", i, j);
            scanf("%d", &matrix[i][j]);
        }
    }
    for (int i = 0; i < 3; i += 1)
    {
        int row_sum = 0;
        for (int j = 0; j < 3; j += 1)
        {
            row_sum += matrix[i][j];
        }
        printf("Row %d sum: %d\n", i, row_sum);
        total += row_sum;
    }
    printf("Total: %d\n", total);
    return 0;
}
