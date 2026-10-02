#include <stdio.h>

int main(void)
{
	int num;
	long long factorial = 1;
	scanf("%d", &num);

	if (num<0) {
		puts("No factorial for negatives");
	} else {
		int i = num;
		while (i > 1) {
			// multiply down
			factorial *= i;
			i--;
		}
		printf("%d! = %lld\n", num, factorial);
	}
	return 0;
}
