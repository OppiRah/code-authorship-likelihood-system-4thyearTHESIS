#include <stdio.h>

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
  int pc=0,psum=0;
  for(int i=a; i<=b; i+=1) {
    if(prime(i)) {
      printf("%d ",i);
      pc+=1;
      psum+=i;
    }
  }
  printf("\nCount: %d, Sum: %d\n",pc,psum);
  int c=0;
  for(int i=a; i<=b; i+=1) {
    if(i>1&&!prime(i)) {
      c+=1;
    }
  }
  printf("Composite numbers: %d\n",c);
  printf("Numbers checked: %d\n",b-a+1);
  return 0;
}

int prime(int x) {
  if(x<2) {
    return 0;
  }
  for(int i=2; i<x; i+=1) {
    if(x%i==0) {
      return 0;
    }
  }
  return 1;
}
