#include <stdio.h>

int prime(int x) {
  if(x<2) {
    return 0;
  }
  for(int i=2; i < x; ++i) {
    if(x%i==0) {
      // found a divisor
      return 0;
    }
  }
  return 1;
}

int lp(int a, int b)
{
  int pc = 0;
  for(int i=a; i <= b; ++i) {
    if(prime(i))
    {
      printf("%d ", i);
      ++pc;
    }
  }
  printf("\n");
  return pc;
}

int main() {
  int a, b;
  scanf("%d %d", &a, &b);
  if(a > b) {
    int t=a;
    a = b;
    b=t;
  }
  int pc = lp(a, b);
  printf("%d primes found\n", pc);

  int c = 0;
  for(int i=a; i <= b; ++i)
  {
    if(i>1 && !prime(i))
    {
      c+=1;
    }
  }
  printf("Composite numbers: %d\n", c);

  int mx=-1;
  for(int i = b; i>=a; i--)
  {
    if(prime(i)) {
      mx = i;
      break;
    }
  }
  printf("Largest prime: %d\n", mx);
  printf("Numbers checked: %d\n", b - a+1);
  return 0;
}
