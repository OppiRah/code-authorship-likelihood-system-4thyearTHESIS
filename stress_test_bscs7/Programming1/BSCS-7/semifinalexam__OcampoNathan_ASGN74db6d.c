#include <stdio.h>

int main() {
  int x,t,d=0,ds=0,rev=0;
  printf("Enter a number: ");
  scanf("%d",&x);

  t = x;
  while(t!=0) {
    d+=1;
    t = t/10;
  }
  t = x;
  while(t!=0) {
    ds=ds+t%10;
    rev = rev*10+t%10;
    t=t/10;
  }
  printf("Digits: %d\n",d);
  printf("Sum of digits: %d\n",ds);
  if(rev==x) {
    printf("It is a palindrome\n");
  } else {
    printf("It is not a palindrome\n");
  }
  printf("\n--- Results ---\n");
  return 0;
}
