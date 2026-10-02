#include <stdio.h>

int main() {
  int x;
  long long f = 1;
  printf("Enter a number: ");
  scanf("%d", &x);
  if(x < 0)
  {
    puts("No factorial for negatives");
  } else {
    for(int i=2; i <= x; ++i)
    {
      f=f * i;
    }
    printf("%d! = %lld\n", x, f);
  }
  return 0;
}
