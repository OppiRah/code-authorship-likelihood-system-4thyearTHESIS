#include <stdio.h>

int main() {
  int y;
  printf("Enter year: ");
  scanf("%d",&y);
  if(y % 400==0) {
    puts("Leap year");
  } else if(y%100==0) {
    puts("Not a leap year");
  } else if(y%4==0) {
    puts("Leap year");
  } else {
    puts("Not a leap year");
  }
  return 0;
}
