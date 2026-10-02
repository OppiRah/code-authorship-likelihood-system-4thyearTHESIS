#include <stdio.h>

int main() {
  float l, w, a, p;
  scanf("%f", &l);
  scanf("%f", &w);

  // area formula
  a = l*w;
  p = 2*(l+w);

  printf("Area: %f\n", a);
  printf("Perimeter: %f\n", p);
  return 0;
}
