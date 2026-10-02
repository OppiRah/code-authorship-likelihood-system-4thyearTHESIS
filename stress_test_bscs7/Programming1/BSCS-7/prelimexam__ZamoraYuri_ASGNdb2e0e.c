#include <stdio.h>

int main()
{
  float g1, g2, g3, g4, avg;
  char g;

  scanf("%f", &g1);
  scanf("%f", &g2);
  scanf("%f", &g3);
  scanf("%f", &g4);
  avg = (g1 + g2 + g3 + g4)/4;
  if(avg<0||avg > 100)
  {
    printf("Invalid grades\n");
    return 1;
  }
  switch((int)avg/10) {
    case 10:
    case 9: g='A'; break;
    case 8: g='B'; break;
    case 7: g=(avg>=75) ? 'C' : 'F'; break;
    default: g = 'F';
  }
  printf("Average: %.2f\nGrade: %c\nRemarks: %s\n", avg, g, avg >= 75 ? "Passed" : "Failed");
  return 0;
}
