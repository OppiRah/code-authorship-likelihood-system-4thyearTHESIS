#include <stdio.h>

int main() {
  float g1,g2,g3,g4,avg;
  char g;
  printf("Enter grade: ");
  scanf("%f",&g1);
  printf("Enter grade: ");
  scanf("%f",&g2);
  printf("Enter grade: ");
  scanf("%f",&g3);
  printf("Enter grade: ");
  scanf("%f",&g4);
  avg=(g1+g2+g3+g4)/4;
  if(avg<0||avg>100) {
    printf("Invalid grades\n");
    return 1;
  }
  if(avg>=90) {
    g='A';
  } else if(avg>=80) {
    g='B';
  } else if(avg>=75) {
    g='C';
  } else {
    g='F';
  }
  printf("\n--- Results ---\n");
  printf("Average: %.2f\n",avg);
  printf("Grade: %c\n",g);
  printf("Remarks: %s\n",avg >= 75 ? "Passed" : "Failed");
  return 0;
}
