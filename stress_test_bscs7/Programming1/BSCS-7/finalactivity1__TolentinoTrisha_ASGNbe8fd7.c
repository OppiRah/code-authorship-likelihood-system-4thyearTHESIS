#include <stdio.h>

int main() {
  int arr[50],n;
  scanf("%d",&n);
  for(int i=0; i<n; i++) {
    scanf("%d",&arr[i]);
  }
  int s=0;
  for(int i=0; i<n; i++) {
    s+=arr[i];
  }
  printf("Sum = %d\n",s);
  printf("Average = %.2f\n",(float)s/n);
  return 0;
}
