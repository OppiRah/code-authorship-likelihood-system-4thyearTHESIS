#include <stdio.h>

int main() {
  double a,b;
  char op;
  printf("Enter first number: ");
  scanf("%lf",&a);
  printf("Enter operator (+ - * /): ");
  scanf(" %c",&op);
  printf("Enter second number: ");
  scanf("%lf",&b);
  switch(op) {
    case '+':
    printf("Result: %.2f\n",a+b);
    break;
    case '-':
    printf("Result: %.2f\n",a-b);
    break;
    case '*':
    printf("Result: %.2f\n",a*b);
    break;
    case '/':
    if(b==0) {
      printf("Cannot divide by zero\n");
    } else {
      printf("Result: %.2f\n",a/b);
    }
    break;
    default:
    printf("Unknown operator\n");
  }
  return 0;
}
