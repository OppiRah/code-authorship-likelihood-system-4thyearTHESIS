#include <stdio.h>

int main()
{
  int arr[50], n;
  scanf("%d", &n);
  for(int i = 0; i < n; i += 1) {
    scanf("%d", &arr[i]);
  }
  printf("You entered: ");
  for(int i=0; i < n; i+=1)
  {
    printf("%d ", arr[i]);
  }
  printf("\n");
  int mx = arr[0], mn=arr[0], s=0;
  for(int i=0; i<n; i += 1) {
    if(arr[i] > mx) {
      mx=arr[i];
    }
    else if(arr[i]<mn)
    {
      mn = arr[i];
    }
  }
  for(int i = 0; i<n; i+=1)
  {
    s = s+arr[i];
  }
  float avg = (float)s/n;

  printf("Max: %d\n", mx);
  printf("Min: %d\n", mn);
  printf("Average: %.2f\n", avg);
  return 0;
}
