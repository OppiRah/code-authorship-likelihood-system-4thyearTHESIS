#include <stdio.h>

int main() {
  int n,s=0;
  printf("Enter n: ");
  scanf("%d",&n);
  for(int i=1; i<=n; ++i) {
    s=s+i;
  }
  printf("Sum: %d\n",s);
  printf("Average: %.2f\n",s*1.0/n);
  return 0;
}
