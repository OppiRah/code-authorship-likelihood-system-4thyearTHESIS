#include <stdio.h>

int main()
{
  int x, t, d=0, ds = 0, rev=0;
  scanf("%d", &x);

  t = x;
  for(; t!=0; t = t / 10)
  {
    d++;
  }
  // sum and reverse
  t=x;
  for(; t != 0; t=t/10) {
    ds = ds+t%10;
    rev=rev*10+t % 10;
  }
  printf("Digits: %d\n", d);
  printf("Sum of digits: %d\n", ds);
  printf("Reversed: %d\n", rev);
  if(rev == x) {
    printf("It is a palindrome\n");
  } else {
    printf("It is not a palindrome\n");
  }
  return 0;
}
