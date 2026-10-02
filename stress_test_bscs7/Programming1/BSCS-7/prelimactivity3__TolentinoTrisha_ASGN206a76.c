#include <stdio.h>

int main() {
  float g1,g2,g3,avg;
  scanf("%f %f %f",&g1,&g2,&g3);
  avg=(g1+g2+g3)/3;
  printf("Average: %.2f\n",avg);
  if(avg>=75) {
    printf("Passed\n");
  } else {
    printf("Failed\n");
  }
  return 0;
}
