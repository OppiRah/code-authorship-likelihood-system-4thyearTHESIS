#include <stdio.h>

int main() {
  float a,b,r;
  char op;
  printf("Enter first number: ");
  scanf("%f",&a);
  printf("Enter operator (+ - * /): ");
  scanf(" %c",&op);
  printf("Enter second number: ");
  scanf("%f",&b);
  if(op=='+') {
    r=a+b;
    printf("Result: %.2f\n",r);
  } else if(op=='-') {
    r=a-b;
    printf("Result: %.2f\n",r);
  } else if(op=='*') {
    r=a*b;
    printf("Result: %.2f\n",r);
  } else if(op=='/') {
    if(b!=0) {
      r=a/b;
      printf("Result: %.2f\n",r);
    } else {
      printf("Cannot divide by zero\n");
    }
  } else {
    printf("Unknown operator\n");
  }
  return 0;
}
