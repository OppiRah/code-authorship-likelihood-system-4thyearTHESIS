#include <stdio.h>

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  for(int i=0; i<n; ++i) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }
  printf("You entered: ");
  for(int i=0; i<n; ++i) {
    printf("%d ",arr[i]);
  }
  printf("\n");
  int mx=arr[0],mn=arr[0],s=0;
  for(int i=0; i<n; ++i) {
    if(arr[i]>mx) {
      mx=arr[i];
    } else if(arr[i]<mn) {
      mn=arr[i];
    }
  }
  for(int i=0; i<n; ++i) {
    s=s+arr[i];
  }
  float avg=s*1.0/n;
  int ab=0;
  for(int i=0; i<n; ++i) {
    if(arr[i]>avg) {
      ++ab;
    }
  }
  printf("Max: %d\nMin: %d\nAverage: %.2f\nAbove average: %d\n",mx,mn,avg,ab);
  return 0;
}
