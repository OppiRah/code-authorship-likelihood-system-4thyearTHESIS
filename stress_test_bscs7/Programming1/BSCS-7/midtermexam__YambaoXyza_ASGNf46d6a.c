#include <stdio.h>

int main()
{
  float a, b, r;
  char op;
  char again = 'y';

  while(again == 'y'||again == 'Y')
  {
    scanf("%f", &a);
    scanf(" %c", &op);
    scanf("%f", &b);
    if(op=='+')
    {
      r=a+b;
      printf("Result: %.2f\n", r);
    } else if(op=='-') {
      r=a - b;
      printf("Result: %.2f\n", r);
    }
    else if(op == '*')
    {
      r=a*b;
      printf("Result: %.2f\n", r);
    } else if(op=='/') {
      if(b!=0) {
        r = a/b;
        printf("Result: %.2f\n", r);
      }
      else
      {
        printf("Cannot divide by zero\n");
      }
    } else {
      printf("Unknown operator\n");
    }
    scanf(" %c", &again);
  }
  return 0;
}
