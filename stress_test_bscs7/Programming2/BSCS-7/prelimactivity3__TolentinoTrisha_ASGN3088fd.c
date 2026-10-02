#include <stdio.h>

void sw(int *a,int *b) {
  int t=*a;
  *a=*b;
  *b=t;
}

int main() {
  int a2,b2;
  scanf("%d %d",&a2,&b2);
  printf("Before: %d %d\n",a2,b2);
  sw(&a2,&b2);
  printf("After: %d %d\n",a2,b2);
  return 0;
}
