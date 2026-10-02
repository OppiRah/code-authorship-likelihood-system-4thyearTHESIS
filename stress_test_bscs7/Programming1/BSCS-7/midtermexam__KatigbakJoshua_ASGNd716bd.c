#include <stdio.h>

int main(void)
{
	float num1, num2;
	char operation;

	printf("Enter first number: ");
	scanf("%f", &num1);
	printf("Enter operator (+ - * /): ");
	scanf(" %c", &operation);
	printf("Enter second number: ");
	scanf("%f", &num2);
	switch (operation) {
		case '+':
		printf("Result: %.2f\n", num1 + num2);
		break;
		case '-':
		printf("Result: %.2f\n", num1 - num2);
		break;
		case '*':
		printf("Result: %.2f\n", num1 * num2);
		break;
		case '/':
		if (num2 == 0) {
			printf("Cannot divide by zero\n");
		} else {
			printf("Result: %.2f\n", num1 / num2);
		}
		break;
		default:
		printf("Unknown operator\n");
	}
	return 0;
}
