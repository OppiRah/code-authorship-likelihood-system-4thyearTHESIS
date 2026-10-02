// Isabel Jimenez - Matrix Row Sums

#include <stdio.h>

int main() {
	int matrix[3][3], total = 0;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			printf("m[%d][%d]: ", i, j);
			scanf("%d", &matrix[i][j]);
		}
	}
	// row sums and grand total
	for (int i = 0; i < 3; ++i) {
		int rowSum = 0;
		for (int j = 0; j < 3; ++j) {
			rowSum += matrix[i][j];
		}
		printf("Row %d sum: %d\n", i, rowSum);
		total += rowSum;
	}
	printf("Total: %d\n", total);
	return 0;
}
