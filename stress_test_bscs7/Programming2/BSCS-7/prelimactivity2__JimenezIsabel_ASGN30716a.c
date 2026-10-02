// Isabel Jimenez - Power Function

#include <stdio.h>

long raisePower(int base, int exponent) {
	long result = 1;
	for (int i = 0; i < exponent; ++i) {
		result*=base;
	}
	return result;
}

int main() {
	int base, exponent;
	printf("Base and exponent: ");
	scanf("%d %d", &base, &exponent);
	printf("%d^%d = %ld\n", base, exponent, raisePower(base, exponent));
	return 0;
}
