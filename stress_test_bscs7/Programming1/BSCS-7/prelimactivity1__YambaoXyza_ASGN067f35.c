#include <stdio.h>

int main() {
  float l, w, a, p;
  // get the dimensions
  scanf("%f", &l);
  scanf("%f", &w);

  a = l*w;
  p=2 * (l + w);
  printf("Area: %f\n", a);
  printf("Perimeter: %f\n", p);
  return 0;
}
