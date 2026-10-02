#include <stdio.h>

int prime(int x);
int lp(int a, int b);

int main() {
  int a, b;
  scanf("%d %d", &a, &b);
  if(a > b) {
    int t = a;
    a = b;
    b=t;
  }
  int pc = lp(a, b);
  printf("%d primes found\n", pc);
  printf("Numbers checked: %d\n", b - a+1);

  int mx=-1;
  for(int i=b; i>=a; i--) {
    if(prime(i))
    {
      mx=i;
      break;
    }
  }
  printf("Largest prime: %d\n", mx);
  int c=0;
  for(int i = a; i<=b; i+=1)
  {
    if(i>1 && !prime(i)) {
      c=c+1;
    }
  }
  printf("Composite numbers: %d\n", c);
  return 0;
}

int prime(int x) {
  if(x < 2) {
    return 0;
  }
  for(int i = 2; i <= x / 2; i+=1)
  {
    if(x%i == 0) {
      return 0;
    }
  }
  return 1;
}

int lp(int a, int b)
{
  int pc=0;
  for(int i=a; i<=b; i += 1) {
    if(prime(i)) {
      printf("%d ", i);
      pc+=1;
    }
  }
  printf("\n");
  return pc;
}
