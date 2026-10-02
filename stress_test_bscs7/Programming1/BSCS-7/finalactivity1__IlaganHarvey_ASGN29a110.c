#include <stdio.h>

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  for(int i=0; i<n; i++) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }
  int s=0;
  for(int i=0; i<n; i++) {
    s+=arr[i];
  }
  printf("Sum = %d\n",s);
  printf("Average = %.2f\n",s*1.0/n);
  return 0;
}
