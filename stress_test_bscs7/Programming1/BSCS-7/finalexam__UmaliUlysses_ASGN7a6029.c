#include <stdio.h>

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  if(n<=0||n>50) {
    printf("Invalid size\n");
    return 1;
  }
  for(int i=0; i<n; i++) {
    printf("Enter number %d: ",i + 1);
    scanf("%d",&arr[i]);
  }
  int mx=arr[0],mn=arr[0],s=0;
  for(int i=0; i<n; i++) {
    if(arr[i]>mx) {
      mx=arr[i];
    } else if(arr[i]<mn) {
      mn=arr[i];
    }
  }
  for(int i=0; i<n; i++) {
    s+=arr[i];
  }
  float avg = (float)s/n;
  int ab=0;
  for(int i=0; i<n; i++) {
    if(arr[i]>avg) {
      ab++;
    }
  }

  printf("Max: %d\n",mx);
  printf("Min: %d\n",mn);
  printf("Average: %.2f\n",avg);
  printf("Above average: %d\n",ab);
  return 0;
}
