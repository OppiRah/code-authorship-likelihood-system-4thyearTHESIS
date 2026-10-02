// Programming exercise: Temperature Converter
// Submitted by: Erika Marquez

#include <stdio.h>

int main()
{
  float cel = 25.0;
  float fah = (cel * 9 / 5) + 32;
  printf("%.1fC = %.1fF\n", cel, fah);
  return 0;
}
