#include <stdio.h>

int main() {
  int x;
  long long f = 1;
  scanf("%d", &x);

  if(x < 0) {
    printf("No factorial for negatives\n");
  } else {
    int i=x;
    while(i > 1)
    {
      f *= i;
      i--;
    }
    printf("%d! = %lld\n", x, f);
  }
  return 0;
}
