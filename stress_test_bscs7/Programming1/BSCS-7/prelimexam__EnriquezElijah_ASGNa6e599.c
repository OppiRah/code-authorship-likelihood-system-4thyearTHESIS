#include <stdio.h>

int main() {
  float g1, g2, g3, avg;
  char g;

  // input
  scanf("%f", &g1);
  scanf("%f", &g2);
  scanf("%f", &g3);
  avg = (g1+g2+g3) / 3;
  switch((int)avg/10) {
    case 10:
    case 9: g = 'A'; break;
    case 8: g = 'B'; break;
    case 7: g = (avg>=75) ? 'C' : 'F'; break;
    default: g='F';
  }

  printf("Average: %.2f\n", avg);
  if(avg>=75) {
    printf("Remarks: Passed\n");
  }
  else
  {
    printf("Remarks: Failed\n");
  }
  return 0;
}
