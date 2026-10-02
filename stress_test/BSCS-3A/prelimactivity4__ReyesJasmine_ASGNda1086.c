// Seatwork: Factorial Calculator
// Submitted by: Jasmine Reyes

#include <stdio.h>

int main()
{
	int num = 6;
	long fact = 1;
	for (int i = 1; i<=num; i++) {
		fact = fact*i;
	}
	printf("%d! = %ld\n", num, fact);
	return 0;
}
