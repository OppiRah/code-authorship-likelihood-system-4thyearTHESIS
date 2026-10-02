#include <stdio.h>

int main() {
  int x;
  long long f=1;
  printf("Enter a number: ");
  scanf("%d",&x);

  if(x<0) {
    printf("No factorial for negatives\n");
  } else {
    for(int i=2; i<=x; i++) {
      f*=i;
    }
    printf("%d! = %lld\n",x,f);
  }
  return 0;
}
