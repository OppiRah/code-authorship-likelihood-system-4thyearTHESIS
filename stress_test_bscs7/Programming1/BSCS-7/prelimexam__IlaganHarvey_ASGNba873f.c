#include <stdio.h>

int main() {
  float g1,g2,g3,avg;
  char g;
  printf("Enter grade: ");
  scanf("%f",&g1);
  printf("Enter grade: ");
  scanf("%f",&g2);
  printf("Enter grade: ");
  scanf("%f",&g3);
  avg=(g1+g2+g3)/3;
  if(avg<0||avg>100) {
    printf("Invalid grades\n");
    return 1;
  }
  switch((int)avg/10) {
    case 10:
    case 9: g='A'; break;
    case 8: g='B'; break;
    case 7: g=(avg>=75) ? 'C' : 'F'; break;
    default: g='F';
  }
  printf("Average: %.2f\n",avg);
  printf("Grade: %c\n",g);
  if(avg>=75) {
    printf("Remarks: Passed\n");
  } else {
    printf("Remarks: Failed\n");
  }
  printf("\n--- Results ---\n");
  return 0;
}
