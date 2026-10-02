#include <stdio.h>

int main() {
  int y;
  scanf("%d", &y);
  // leap year rule
  if(y % 400 == 0) {
    printf("Leap year\n");
  } else if(y % 100==0) {
    printf("Not a leap year\n");
  } else if(y%4==0) {
    printf("Leap year\n");
  }
  else
  {
    printf("Not a leap year\n");
  }
  return 0;
}
