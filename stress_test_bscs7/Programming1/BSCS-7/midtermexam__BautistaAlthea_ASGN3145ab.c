#include <stdio.h>

int main()
{
  float a, b;
  char op;
  char again = 'y';

  do {
    printf("Enter first number: ");
    scanf("%f", &a);
    printf("Enter operator (+ - * /): ");
    scanf(" %c", &op);
    printf("Enter second number: ");
    scanf("%f", &b);
    switch(op)
    {
      case '+':
      printf("Result: %.2f\n", a + b);
      break;
      case '-':
      printf("Result: %.2f\n", a - b);
      break;
      case '*':
      printf("Result: %.2f\n", a * b);
      break;
      case '/':
      if(b==0)
      {
        puts("Cannot divide by zero");
      }
      else
      {
        printf("Result: %.2f\n", a / b);
      }
      break;
      default:
      puts("Unknown operator");
    }
    printf("Again? (y/n): ");
    scanf(" %c", &again);
  } while(again=='y'||again=='Y');
  return 0;
}
