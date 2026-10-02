#include <stdio.h>

int main() {
  int x;
  printf("Enter a number: ");
  scanf("%d", &x);
  if(x % 2==0) {
    printf("%d is even\n", x);
  } else {
    printf("%d is odd\n", x);
  }
  // check the sign
  if(x>0) {
    puts("positive");
  } else if(x<0) {
    puts("negative");
  } else {
    puts("zero");
  }
  return 0;
}
