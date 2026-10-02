#include <stdio.h>

int main() {
  float g[3],t=0,avg;
  char g2;
  for(int i = 0; i<3; i++) {
    printf("Grade %d: ",i+1);
    scanf("%f",&g[i]);
    t=t+g[i];
  }
  avg=t/3;
  if(avg>=90) {
    g2='A';
  } else if(avg>=80) {
    g2='B';
  } else if(avg>=75) {
    g2 = 'C';
  } else {
    g2='F';
  }
  printf("Average: %.2f\nGrade: %c\nRemarks: %s\n",avg,g2,avg>=75 ? "Passed" : "Failed");
  return 0;
}
