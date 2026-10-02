#include <stdio.h>

int main() {
  int x,t,d=0,ds=0,rev=0;
  printf("Enter a number: ");
  scanf("%d",&x);
  t=x;
  while(t > 0) {
    int r=t%10;
    d++;
    ds+=r;
    rev*=10+r;
    t/=10;
  }
  printf("Digits: %d\n",d);
  printf("Sum of digits: %d\n",ds);
  printf("Reversed: %d\n",rev);
  if(rev==x) {
    printf("It is a palindrome\n");
  } else {
    printf("It is not a palindrome\n");
  }
  printf("\n--- Results ---\n");
  return 0;
}
