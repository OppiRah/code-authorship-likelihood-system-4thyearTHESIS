#include <stdio.h>

int lp(int a,int b);
int prime(int x);

int main() {
  int a,b;
  printf("Enter range (start end): ");
  scanf("%d %d",&a,&b);
  if(a>b) {
    int t=a;
    a=b;
    b=t;
  }
  int pc=lp(a,b);
  printf("%d primes found\n",pc);
  int t2=0;
  for(int i=a; i+2<=b; i+=1) {
    if(prime(i)&&prime(i+2)) {
      t2+=1;
    }
  }
  printf("Twin prime pairs: %d\n",t2);
  int c=0;
  for(int i=a; i<=b; i+=1) {
    if(i>1&&!prime(i)) {
      c=c+1;
    }
  }
  printf("Composite numbers: %d\n",c);
  return 0;
}

int lp(int a,int b) {
  int pc=0;
  for(int i=a; i<=b; i+=1) {
    if(prime(i)) {
      printf("%d ",i);
      pc+=1;
    }
  }
  printf("\n");
  return pc;
}

int prime(int x) {
  if(x<2) {
    return 0;
  }
  for(int i=2; i<=x/2; i+=1) {
    if(x%i==0) {
      return 0;
    }
  }
  return 1;
}
