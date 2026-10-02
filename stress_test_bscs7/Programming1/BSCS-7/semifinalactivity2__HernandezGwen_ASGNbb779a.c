#include <stdio.h>

int main(void)
{
	int num;
	scanf("%d", &num);
	// print the table up to 10
	for (int i = 1; i <= 10; i++) {
		printf("%d x %d = %d\n", num, i, num * i);
	}
	return 0;
}
