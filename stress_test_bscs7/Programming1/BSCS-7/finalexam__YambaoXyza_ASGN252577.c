#include <stdio.h>

int main()
{
  int arr[50], n;
  scanf("%d", &n);
  if(n<=0||n>50) {
    printf("Invalid size\n");
    return 1;
  }
  for(int i = 0; i<n; i++) {
    scanf("%d", &arr[i]);
  }
  int mx = arr[0], mn = arr[0], s = 0;
  for(int i=0; i < n; i++) {
    if(arr[i] > mx)
    {
      mx=arr[i];
    } else if(arr[i]<mn) {
      mn=arr[i];
    }
  }
  for(int i = 0; i < n; i++) {
    s = s + arr[i];
  }
  float avg=(float)s/n;
  printf("Max: %d\nMin: %d\nAverage: %.2f\n", mx, mn, avg);
  return 0;
}
