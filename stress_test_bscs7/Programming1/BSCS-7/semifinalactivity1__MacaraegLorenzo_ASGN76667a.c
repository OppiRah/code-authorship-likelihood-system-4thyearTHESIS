#include <stdio.h>

int main() {
  int n,s=0;
  printf("Enter n: ");
  scanf("%d",&n);
  int i=1;
  while(i <= n) {
    s+=i;
    i+=1;
  }
  printf("Sum: %d\n",s);
  printf("Average: %.2f\n",s*1.0/n);
  return 0;
}
