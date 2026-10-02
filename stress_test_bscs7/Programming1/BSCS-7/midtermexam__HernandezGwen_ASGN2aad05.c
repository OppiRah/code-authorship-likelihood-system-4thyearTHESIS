#include <stdio.h>

int main(void)
{
	float num1, num2, result;
	char operation;
	char repeat = 'y';

	while (repeat == 'y' || repeat == 'Y') {
		// read the expression
		scanf("%f", &num1);
		scanf(" %c", &operation);
		scanf("%f", &num2);
		if (operation == '/' && num2 == 0) {
			puts("Error: division by zero");
		} else {
			switch (operation) {
				case '+':
				result = num1 + num2;
				printf("Result: %.2f\n", result);
				break;
				case '-':
				result = num1 - num2;
				printf("Result: %.2f\n", result);
				break;
				case '*':
				result = num1 * num2;
				printf("Result: %.2f\n", result);
				break;
				case '/':
				result = num1 / num2;
				printf("Result: %.2f\n", result);
				break;
				default:
				puts("Unknown operator");
			}
		}
		scanf(" %c", &repeat);
	}
	return 0;
}
