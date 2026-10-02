#include <stdio.h>

int main() {
  float g1,g2,g3,avg;
  printf("Enter three grades: ");
  scanf("%f %f %f",&g1,&g2,&g3);
  avg=(g1+g2+g3)/3;
  printf("Average: %.2f\n",avg);
  if(avg>=75) {
    puts("Passed");
  } else {
    puts("Failed");
  }
  return 0;
}
