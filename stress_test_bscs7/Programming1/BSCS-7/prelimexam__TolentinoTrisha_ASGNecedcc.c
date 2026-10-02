#include <stdio.h>

int main() {
  float g[3],t=0,avg;
  char g2;
  for(int i=0; i<3; i++) {
    scanf("%f",&g[i]);
    t+=g[i];
  }
  avg=t/3;
  if(avg<0||avg>100) {
    printf("Invalid grades\n");
    return 1;
  }
  if(avg>=90) {
    g2='A';
  } else if(avg>=80) {
    g2='B';
  } else if(avg>=75) {
    g2='C';
  } else {
    g2='F';
  }
  printf("Average: %.2f\n",avg);
  if(avg>=75) {
    printf("Remarks: Passed\n");
  } else {
    printf("Remarks: Failed\n");
  }
  return 0;
}
