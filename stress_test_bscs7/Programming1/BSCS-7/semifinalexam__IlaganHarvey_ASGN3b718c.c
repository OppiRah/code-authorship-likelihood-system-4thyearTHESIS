#include <stdio.h>

int main() {
  int x,t,d=0,ds=0,rev=0;
  printf("Enter a number: ");
  scanf("%d",&x);
  if(x<0) {
    x=-x;
  }
  t=x;
  for(; t; t=t/10) {
    d++;
  }
  t=x;
  for(; t; t=t/10) {
    ds=ds+t%10;
    rev=rev*10+t%10;
  }
  printf("Digits: %d\n",d);
  printf("Sum of digits: %d\n",ds);
  if(rev==x) {
    printf("It is a palindrome\n");
  } else {
    printf("It is not a palindrome\n");
  }
  return 0;
}
