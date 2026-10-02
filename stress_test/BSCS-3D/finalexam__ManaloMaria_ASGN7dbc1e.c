#include <stdio.h>

int main() {
	// compute total inventory value using parallel arrays
	char names[3][20] = { "Pen", "Notebook", "Eraser" };
	int qty[3] = { 10, 4, 15 };
	float price[3] = { 5.5, 25.0, 3.0 };
	float grand_total = 0;
	for (int i = 0; i < 3; i++) {
		printf("%s: %d x %.2f\n", names[i], qty[i], price[i]);
		grand_total += qty[i] * price[i];
	}
	printf("Total: %.2f\n", grand_total);
	return 0;
}
