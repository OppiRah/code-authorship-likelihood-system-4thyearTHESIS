#include <stdio.h>

int main() {
  int x;
  long long f=1;
  printf("Enter a number: ");
  scanf("%d",&x);
  if(x<0) {
    puts("No factorial for negatives");
  } else {
    int i=x;
    while(i>1) {
      f*=i;
      i--;
    }
    printf("%d! = %lld\n",x,f);
  }
  return 0;
}
