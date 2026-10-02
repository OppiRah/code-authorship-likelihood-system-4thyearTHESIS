#include <stdio.h>

int main() {
  float v, t = 0, avg;
  char g;
  int i=0;
  while(i < 3) {
    scanf("%f", &v);
    t = t + v;
    i++;
  }
  avg = t / 3;
  // letter grade
  if(avg < 75)
  {
    g = 'F';
  }
  else if(avg < 80)
  {
    g = 'C';
  } else if(avg < 90) {
    g='B';
  }
  else
  {
    g = 'A';
  }
  printf("Average: %.2f\n", avg);
  if(avg>=75)
  {
    printf("Remarks: Passed\n");
  }
  else
  {
    printf("Remarks: Failed\n");
  }
  return 0;
}
