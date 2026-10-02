// Faith Flores - Matrix Row Sums

#include <stdio.h>

int main() {
    int matrix[3][3], total = 0;
    for (int i = 0; i < 3; i += 1) {
        for (int j = 0; j < 3; j += 1) {
            printf("m[%d][%d]: ", i, j);
            scanf("%d", &matrix[i][j]);
        }
    }
    // row sums and grand total
    for (int i = 0; i < 3; i += 1) {
        int rowSum = 0;
        for (int j = 0; j < 3; j += 1) {
            rowSum = rowSum + matrix[i][j];
        }
        printf("Row %d sum: %d\n", i, rowSum);
        total = total+rowSum;
    }
    printf("Total: %d\n", total);
    return 0;
}
