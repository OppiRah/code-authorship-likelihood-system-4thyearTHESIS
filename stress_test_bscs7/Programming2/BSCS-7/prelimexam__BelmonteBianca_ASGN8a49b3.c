#include <stdio.h>

int prime(int x);

int main() {
  int a,b;
  printf("Enter range (start end): ");
  scanf("%d %d",&a,&b);
  int pc=0,psum=0;
  int arr[500];
  for(int i = a; i<=b; i++) {
    if(prime(i)) {
      arr[pc]=i;
      pc++;
      psum=psum + i;
    }
  }
  for(int i=0; i<pc; i++) {
    printf("%d ",arr[i]);
  }
  printf("\nCount: %d, Sum: %d\n",pc,psum);
  int c=0;
  for(int i=a; i<=b; i++) {
    if(i>1&&!prime(i)) {
      c=c+1;
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
