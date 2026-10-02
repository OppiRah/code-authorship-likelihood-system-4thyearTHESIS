#include <stdio.h>

int main() {
  float a,b;
  char op;
  char again='y';
  do {
    printf("Enter first number: ");
    scanf("%f",&a);
    printf("Enter operator (+ - * /): ");
    scanf(" %c",&op);
    printf("Enter second number: ");
    scanf("%f",&b);
    if(op=='/'&&b == 0) {
      printf("Error: division by zero\n");
    } else {
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
        printf("Result: %.2f\n",a/b);
        break;
        default:
        printf("Unknown operator\n");
      }
    }
    printf("Again? (y/n): ");
    scanf(" %c",&again);
  } while(again=='y'||again=='Y');
  return 0;
}
