#include <stdio.h>

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  if(n<=0 || n > 50) {
    printf("Invalid size\n");
    return 1;
  }
  for(int i=0; i<n; i+=1) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }
  printf("You entered: ");
  for(int i = 0; i<n; i+=1) {
    printf("%d ",arr[i]);
  }
  printf("\n");
  int mx=arr[0],mn=arr[0],s=0;
  int i;
  i=0;
  while(i<n) {
    s+=arr[i];
    if(arr[i]>mx) {
      mx=arr[i];
    }
    if(arr[i]<mn) {
      mn=arr[i];
    }
    i+=1;
  }
  float avg=(float)s / n;
  int ab=0;
  for(int i=0; i<n; i+=1) {
    if(arr[i]>avg) {
      ab+=1;
    }
  }
  printf("\n--- Results ---\n");
  printf("Max: %d\n",mx);
  printf("Min: %d\n",mn);
  printf("Average: %.2f\n",avg);
  printf("Above average: %d\n",ab);
  return 0;
}
