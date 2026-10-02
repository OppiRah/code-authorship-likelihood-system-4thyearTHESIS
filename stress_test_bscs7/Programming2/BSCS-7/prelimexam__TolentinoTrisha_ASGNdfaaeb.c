#include <stdio.h>

int prime(int x);

int main() {
  int a,b;
  scanf("%d %d",&a,&b);
  if(a>b) {
    int t=a;
    a=b;
    b=t;
  }
  int pc = 0,psum=0;
  int arr[500];
  for(int i=a; i<=b; i++) {
    if(prime(i)) {
      arr[pc]=i;
      pc++;
      psum+=i;
    }
  }
  for(int i=0; i < pc; i++) {
    printf("%d ",arr[i]);
  }
  printf("\nCount: %d, Sum: %d\n",pc,psum);
  printf("Numbers checked: %d\n",b - a+1);
  int t2=0;
  for(int i=a; i+2<=b; i++) {
    if(prime(i)&&prime(i+2)) {
      t2++;
    }
  }
  printf("Twin prime pairs: %d\n",t2);
  int c=0;
  for(int i=a; i<=b; i++) {
    if(i>1&&!prime(i)) {
      c+=1;
    }
  }
  printf("Composite numbers: %d\n",c);
  return 0;
}

int prime(int x) {
  if(x<2) {
    return 0;
  }
  for(int i=2; i<x; i++) {
    if(x%i==0) {
      return 0;
    }
  }
  return 1;
}
