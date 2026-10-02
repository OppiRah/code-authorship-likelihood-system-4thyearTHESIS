#include <stdio.h>

int main() {
  double a,b;
  char op;
  scanf("%lf",&a);
  scanf(" %c",&op);
  scanf("%lf",&b);
  if(op=='+') {
    printf("Result: %.2f\n",a+b);
  } else if(op=='-') {
    printf("Result: %.2f\n",a-b);
  } else if(op=='*') {
    printf("Result: %.2f\n",a*b);
  } else if(op=='/') {
    if(b!=0) {
      printf("Result: %.2f\n",a/b);
    } else {
      printf("Cannot divide by zero\n");
    }
  } else {
    printf("Unknown operator\n");
  }
  return 0;
}
