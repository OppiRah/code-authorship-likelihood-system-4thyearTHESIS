#include <stdio.h>

int prime(int x);

int main()
{
  int a, b;
  printf("Enter range (start end): ");
  scanf("%d %d", &a, &b);
  if(a>b)
  {
    int t = a;
    a = b;
    b=t;
  }
  int pc=0, psum=0;
  for(int i = a; i <= b; ++i) {
    if(prime(i)) {
      printf("%d ", i);
      ++pc;
      psum=psum + i;
    }
  }
  printf("\nCount: %d, Sum: %d\n", pc, psum);
  int t2 = 0;
  for(int i = a; i+2 <= b; ++i) {
    if(prime(i) && prime(i + 2)) {
      ++t2;
    }
  }
  printf("Twin prime pairs: %d\n", t2);
  return 0;
}

int prime(int x)
{
  if(x < 2)
  {
    return 0;
  }
  for(int i = 2; i<=x / 2; ++i)
  {
    if(x % i == 0) {
      return 0;
    }
  }
  return 1;
}
