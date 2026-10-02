#include <stdio.h>

int main() {
  float g[3],t=0,avg;
  char g2;
  for(int i=0; i<3; i+=1) {
    printf("Grade %d: ",i+1);
    scanf("%f",&g[i]);
    t+=g[i];
  }
  avg=t/3;
  switch((int)avg/10) {
    case 10:
    case 9: g2='A'; break;
    case 8: g2='B'; break;
    case 7: g2=(avg>=75) ? 'C' : 'F'; break;
    default: g2='F';
  }

  printf("\n--- Results ---\n");
  printf("Average: %.2f\nGrade: %c\nRemarks: %s\n",avg,g2,avg>=75 ? "Passed" : "Failed");
  return 0;
}
