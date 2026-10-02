// Class activity: Prime Number Check
// Submitted by: Jose Panganiban

#include <stdio.h>

int main() {
	int num = 29;
	int ip = 1;
	for (int i = 2; i<=num/2; i++) {
		if (num % i == 0) {
			ip = 0;
			break;
		}
	}
	if (ip) printf("%d is prime\n", num);
	else printf("%d is not prime\n", num);
	return 0;
}
