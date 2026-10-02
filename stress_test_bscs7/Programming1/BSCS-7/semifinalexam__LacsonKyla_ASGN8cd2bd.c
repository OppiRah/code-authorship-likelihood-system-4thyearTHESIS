#include <stdio.h>

int main() {
  int x,t,d=0,ds=0,rev=0;
  printf("Enter a number: ");
  scanf("%d",&x);

  t=x;
  for(; t != 0; t=t/10) {
    ++d;
  }
  t=x;
  for(; t!=0; t=t/10) {
    ds=ds+t % 10;
    rev=rev*10+t%10;
  }
  printf("Digits: %d\nSum of digits: %d\nReversed: %d\n%s\n",d,ds,rev,rev==x ? "Palindrome" : "Not a palindrome");
  return 0;
}
