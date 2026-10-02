#include <stdio.h>

int main()
{
  int a, b, c, mx;
  scanf("%d %d %d", &a, &b, &c);
  if(a >= b&&a>=c)
  {
    mx=a;
  } else if(b >= a&&b>=c) {
    mx=b;
  } else {
    mx=c;
  }
  printf("Largest: %d\n", mx);
  return 0;
}
