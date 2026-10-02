#include <stdio.h>

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  for(int i=0; i<n; i++) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }
  int ev=0,od = 0;
  for(int i=0; i<n; i++) {
    if(arr[i]%2==0) {
      ev++;
    } else {
      od++;
    }
  }
  printf("Even: %d, Odd: %d\n",ev,od);
  return 0;
}
