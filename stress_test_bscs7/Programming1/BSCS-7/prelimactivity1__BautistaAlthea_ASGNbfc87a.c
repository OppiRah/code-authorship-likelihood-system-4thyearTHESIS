#include <stdio.h>

int main()
{
  float l, w, a, p;

  printf("Enter length: ");
  scanf("%f", &l);
  printf("Enter width: ");
  scanf("%f", &w);
  a = l*w;
  p=2 * (l + w);
  printf("Area: %f\n", a);
  printf("Perimeter: %f\n", p);
  return 0;
}
