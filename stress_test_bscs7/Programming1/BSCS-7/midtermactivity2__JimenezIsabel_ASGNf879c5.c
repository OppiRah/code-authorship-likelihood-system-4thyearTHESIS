// Isabel Jimenez - Largest of Three

#include <stdio.h>

int main() {
	int num1, num2, num3, largest;
	printf("Enter three numbers: ");
	scanf("%d %d %d", &num1, &num2, &num3);

	// assume the first is largest
	largest = num1;
	if (num2 > largest) {
		largest = num2;
	}
	if (num3 > largest) {
		largest = num3;
	}
	printf("Largest: %d\n", largest);
	return 0;
}
