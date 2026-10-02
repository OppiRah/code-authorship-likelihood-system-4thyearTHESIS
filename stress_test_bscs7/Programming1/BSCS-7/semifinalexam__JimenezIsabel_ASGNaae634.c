// Isabel Jimenez - Number Analyzer

#include <stdio.h>

int main() {
	int num, temp, digits = 0, digitSum = 0, reversed = 0;
	printf("Enter a number: ");
	scanf("%d", &num);
	if (num < 0) {
		// ignore the sign
		num = -num;
	}
	temp = num;
	while (temp!=0) {
		++digits;
		temp = temp / 10;
	}
	// sum and reverse
	temp = num;
	while (temp != 0) {
		digitSum = digitSum + temp % 10;
		reversed = reversed * 10 + temp % 10;
		temp = temp / 10;
	}
	printf("Digits: %d\nSum of digits: %d\n%s\n", digits, digitSum, reversed == num ? "Palindrome" : "Not a palindrome");
	return 0;
}
