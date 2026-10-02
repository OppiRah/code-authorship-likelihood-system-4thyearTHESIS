#include <stdio.h>

int main()
{
  int a, b, c, mx;
  printf("Enter three numbers: ");
  scanf("%d %d %d", &a, &b, &c);
  mx=a;
  if(b>mx)
  {
    mx=b;
  }
  if(c>mx) {
    mx = c;
  }
  printf("Largest: %d\n", mx);
  return 0;
}
