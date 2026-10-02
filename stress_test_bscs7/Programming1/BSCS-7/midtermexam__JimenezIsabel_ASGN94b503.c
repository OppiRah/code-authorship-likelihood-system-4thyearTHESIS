// Isabel Jimenez - Simple Calculator

#include <stdio.h>

int main() {
	float num1, num2, result;
	char operation;
	char repeat = 'y';
	do {
		printf("Enter first number: ");
		scanf("%f", &num1);
		printf("Enter operator (+ - * /): ");
		scanf(" %c", &operation);
		printf("Enter second number: ");
		scanf("%f", &num2);
		if (operation == '/'&&num2 == 0) {
			puts("Error: division by zero");
		}
		else {
			if (operation == '+') {
				result = num1 + num2;
				printf("Result: %.2f\n", result);
			}
			else if (operation == '-') {
				result = num1 - num2;
				printf("Result: %.2f\n", result);
			}
			else if (operation == '*') {
				result = num1 * num2;
				printf("Result: %.2f\n", result);
			}
			else if (operation == '/') {
				result = num1 / num2;
				printf("Result: %.2f\n", result);
			}
			else {
				puts("Unknown operator");
			}
		}
		printf("Again? (y/n): ");
		scanf(" %c", &repeat);
	} while (repeat == 'y' || repeat == 'Y');
	return 0;
}
