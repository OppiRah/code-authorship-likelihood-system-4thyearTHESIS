#include <stdio.h>

int main()
{
  float a, b;
  char op;
  char again='y';

  while(again == 'y'||again == 'Y') {
    // read the expression
    scanf("%f", &a);
    scanf(" %c", &op);
    scanf("%f", &b);
    if(op == '+') {
      printf("Result: %.2f\n", a + b);
    }
    else if(op == '-')
    {
      printf("Result: %.2f\n", a-b);
    }
    else if(op=='*')
    {
      printf("Result: %.2f\n", a*b);
    } else if(op == '/') {
      if(b != 0)
      {
        printf("Result: %.2f\n", a / b);
      } else {
        printf("Cannot divide by zero\n");
      }
    }
    else
    {
      printf("Unknown operator\n");
    }
    scanf(" %c", &again);
  }
  printf("\n--- Results ---\n");
  return 0;
}
