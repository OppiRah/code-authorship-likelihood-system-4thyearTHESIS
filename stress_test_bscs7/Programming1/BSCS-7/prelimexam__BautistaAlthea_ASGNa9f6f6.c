#include <stdio.h>

int main()
{
  float g1, g2, g3, g4, avg;
  char g;
  printf("Enter grade: ");
  scanf("%f", &g1);
  printf("Enter grade: ");
  scanf("%f", &g2);
  printf("Enter grade: ");
  scanf("%f", &g3);
  printf("Enter grade: ");
  scanf("%f", &g4);
  avg = (g1+g2 + g3+g4)/4;
  if(avg<0||avg>100) {
    puts("Invalid grades");
    return 1;
  }
  if(avg < 75) {
    g='F';
  } else if(avg<80) {
    g = 'C';
  }
  else if(avg < 90)
  {
    g='B';
  } else {
    g = 'A';
  }
  printf("Average: %.2f\nGrade: %c\nRemarks: %s\n", avg, g, avg >= 75 ? "Passed" : "Failed");
  return 0;
}
