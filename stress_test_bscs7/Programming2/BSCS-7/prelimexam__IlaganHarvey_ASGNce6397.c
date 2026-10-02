#include <stdio.h>

int prime(int x) {
  if(x<2) {
    return 0;
  }
  for(int i=2; i<=x/2; i++) {
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
  int pc=lp(a,b);
  printf("%d primes found\n",pc);
  int t=0;
  for(int i=a; i+2<=b; i++) {
    if(prime(i)&&prime(i+2)) {
      t++;
    }
  }
  printf("Twin prime pairs: %d\n",t);
  int c=0;
  for(int i=a; i<=b; i++) {
    if(i>1&&!prime(i)) {
      c=c+1;
    }
  }
  printf("\n--- Results ---\n");
  printf("Composite numbers: %d\n",c);
  return 0;
}
