#include <stdio.h>

int prime(int x) {
  if(x<2) {
    return 0;
  }
  for(int i = 2; i < x; i++) {
    if(x%i==0)
    {
      // found a divisor
      return 0;
    }
  }
  return 1;
}

int main()
{
  int x;
  scanf("%d", &x);
  if(prime(x)) {
    printf("%d is prime\n", x);
  }
  else
  {
    printf("%d is not prime\n", x);
  }
  return 0;
}
