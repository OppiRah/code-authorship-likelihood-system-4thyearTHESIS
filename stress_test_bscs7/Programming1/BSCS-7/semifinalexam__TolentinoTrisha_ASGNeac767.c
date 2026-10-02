#include <stdio.h>

int main() {
  int x,t,d=0,ds=0,rev=0;
  scanf("%d",&x);
  t=x;
  for(; t>0; t/=10) {
    int r=t%10;
    d++;
    ds+=r;
    rev*=10+r;
  }
  if(x == 0) {
    d=1;
  }
  printf("Digits: %d\n",d);
  printf("Sum of digits: %d\n",ds);
  printf("%s\n",rev==x ? "Palindrome" : "Not a palindrome");
  return 0;
}
