#include <stdio.h>

int main() {
  int l,w,a,p;
  scanf("%d",&l);
  scanf("%d",&w);
  a=l*w;
  p=2*(l + w);
  printf("Area: %d\n",a);
  printf("Perimeter: %d\n",p);
  return 0;
}
