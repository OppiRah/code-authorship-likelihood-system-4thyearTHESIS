#include <stdio.h>

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

int main() {
  int x;
  printf("Enter a number: ");
  scanf("%d",&x);
  if(prime(x)) {
    printf("%d is prime\n",x);
  } else {
    printf("%d is not prime\n",x);
  }
  return 0;
}
