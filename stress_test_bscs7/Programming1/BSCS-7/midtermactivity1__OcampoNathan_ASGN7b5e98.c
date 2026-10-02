#include <stdio.h>

int main() {
  int x;
  printf("Enter a number: ");
  scanf("%d",&x);
  if(x%2==0) {
    printf("%d is even\n",x);
  } else {
    printf("%d is odd\n",x);
  }
  if(x>0) {
    printf("positive\n");
  } else if(x<0) {
    printf("negative\n");
  } else {
    printf("zero\n");
  }
  return 0;
}
