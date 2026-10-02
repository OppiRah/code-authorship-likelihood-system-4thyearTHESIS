#include <stdio.h>

int main()
{
  float g1, g2, g3, avg;
  scanf("%f %f %f", &g1, &g2, &g3);
  // average of three
  avg=(g1+g2 + g3) / 3;
  printf("Average: %.2f\n", avg);
  return 0;
}
