#include <stdio.h>

int main() {
  int x, t, d = 0, ds = 0, rev=0;
  scanf("%d", &x);
  if(x < 0)
  {
    x = -x;
  }
  t = x;
  while(t > 0)
  {
    // last digit
    int r=t % 10;
    ++d;
    ds+=r;
    rev*=10 + r;
    t/=10;
  }
  if(x == 0) {
    d=1;
  }

  printf("Digits: %d\n", d);
  printf("Sum of digits: %d\n", ds);
  if(rev==x)
  {
    printf("It is a palindrome\n");
  } else {
    printf("It is not a palindrome\n");
  }
  return 0;
}
