#include <stdio.h>

int main(void)
{
	int num, temp, digits = 0, digit_sum = 0, reversed = 0;
	scanf("%d", &num);
	if (num < 0) {
		num = -num;
	}

	temp=num;
	for (; temp > 0; temp/=10) {
		int remainder = temp % 10;
		digits++;
		digit_sum += remainder;
		reversed = reversed * 10 + remainder;
	}
	printf("Digits: %d\n", digits);
	printf("Sum of digits: %d\n", digit_sum);
	printf("Reversed: %d\n", reversed);
	if (reversed == num) {
		puts("It is a palindrome");
	} else {
		puts("It is not a palindrome");
	}
	return 0;
}
