#include <stdio.h>

int prime(int x) {
  if(x<2) {
    return 0;
  }
  for(int i = 2; i<=x/2; i++) {
    if(x%i==0) {
      return 0;
    }
  }
  return 1;
}

int lp(int a,int b) {
  int pc=0;
  for(int i=a; i<=b; i++) {
    if(prime(i)) {
      printf("%d ",i);
      pc++;
    }
  }
  printf("\n");
  return pc;
}

int main() {
  int a,b;
  printf("Enter range (start end): ");
  scanf("%d %d",&a,&b);
  if(a>b) {
    int t=a;
    a = b;
    b=t;
  }
  int pc = lp(a,b);
  printf("%d primes found\n",pc);

  printf("Numbers checked: %d\n",b-a+1);
  return 0;
}
