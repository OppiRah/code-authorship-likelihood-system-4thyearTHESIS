#include <stdio.h>

long pw(int b,int e) {
  long r=1;
  while(e > 0) {
    r=r*b;
    e--;
  }
  return r;
}

int main() {
  int b,e;
  printf("Base and exponent: ");
  scanf("%d %d",&b,&e);
  printf("%d^%d = %ld\n",b,e,pw(b,e));
  return 0;
}
