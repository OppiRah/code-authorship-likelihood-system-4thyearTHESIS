#include <stdio.h>

int main() {
  int x,t,d=0,ds=0,rev=0;
  printf("Enter a number: ");
  scanf("%d",&x);
  if(x<0) {
    x=-x;
  }
  t=x;
  while(t!=0) {
    d+=1;
    t=t/10;
  }
  t=x;
  while(t!=0) {
    ds=ds + t%10;
    rev=rev*10+t%10;
    t=t/10;
  }
  if(x==0) {
    d=1;
  }
  printf("Digits: %d\nSum of digits: %d\n%s\n",d,ds,rev==x ? "Palindrome" : "Not a palindrome");
  return 0;
}
