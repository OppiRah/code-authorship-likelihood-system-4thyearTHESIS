#include <stdio.h>

int main() {
  int x;
  long long f=1;
  scanf("%d", &x);
  if(x < 0) {
    printf("No factorial for negatives\n");
  }
  else
  {
    for(int i = 2; i<=x; i+=1)
    {
      f=f*i;
    }
    printf("%d! = %lld\n", x, f);
  }
  return 0;
}
