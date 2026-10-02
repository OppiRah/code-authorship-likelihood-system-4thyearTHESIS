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
  if(op=='/'&&b==0) {
    printf("Error: division by zero\n");
  } else {
    if(op=='+') {
      printf("Result: %.2f\n",a+b);
    } else if(op=='-') {
      printf("Result: %.2f\n",a - b);
    } else if(op=='*') {
      printf("Result: %.2f\n",a*b);
    } else if(op=='/') {
      printf("Result: %.2f\n",a/b);
    } else {
      printf("Unknown operator\n");
    }
  }
  printf("\n--- Results ---\n");
  return 0;
}
