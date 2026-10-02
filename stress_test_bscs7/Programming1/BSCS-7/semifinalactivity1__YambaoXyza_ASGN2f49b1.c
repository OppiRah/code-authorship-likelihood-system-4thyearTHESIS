#include <stdio.h>

int main() {
  int n, s = 0;
  scanf("%d", &n);

  for(int i = 1; i <= n; i++) {
    s=s+i;
  }
  printf("Sum: %d\n", s);
  printf("Average: %.2f\n", (float)s/n);
  return 0;
}
