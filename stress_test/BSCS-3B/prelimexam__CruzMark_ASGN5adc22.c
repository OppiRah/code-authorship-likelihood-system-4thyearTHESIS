// Practice exercise: Grade Average and Letter Grade
// Submitted by: Mark Cruz

#include <stdio.h>

char gg(float avg)
{
  if (avg >= 90) return 'A';
  if (avg >= 80) return 'B';
  if (avg >= 70) return 'C';
  return 'F';
}

int main()
{
  // compute average of 5 scores using a helper function
  int sco[5] = { 88, 92, 76, 81, 95 };
  int total = 0;
  for (int i = 0; i < 5; i++) total += sco[i];
  float ave = (float)total / 5;
  char let = gg(ave);
  printf("Average: %.2f, Grade: %c\n", ave, let);
  return 0;
}
