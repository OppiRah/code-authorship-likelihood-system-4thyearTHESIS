#include <stdio.h>

int main() {
  float a,b,r;
  char op;
  char again='y';
  do {
    printf("Enter first number: ");
    scanf("%f",&a);
    printf("Enter operator (+ - * /): ");
    scanf(" %c",&op);
    printf("Enter second number: ");
    scanf("%f",&b);
    if(op=='/'&&b==0) {
      printf("Error: division by zero\n");
    } else {
      switch(op) {
        case '+':
        r=a+b;
        printf("Result: %.2f\n",r);
        break;
        case '-':
        r=a-b;
        printf("Result: %.2f\n",r);
        break;
        case '*':
        r=a*b;
        printf("Result: %.2f\n",r);
        break;
        case '/':
        r=a/b;
        printf("Result: %.2f\n",r);
        break;
        default:
        printf("Unknown operator\n");
      }
    }
    printf("Again? (y/n): ");
    scanf(" %c",&again);
  } while(again=='y' || again=='Y');
  return 0;
}
