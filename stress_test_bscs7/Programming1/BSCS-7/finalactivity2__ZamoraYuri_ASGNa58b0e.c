#include <stdio.h>

int main()
{
  int arr[50], n;
  scanf("%d", &n);
  for(int i = 0; i<n; ++i) {
    scanf("%d", &arr[i]);
  }
  int mx = arr[0], p=0;
  for(int i = 1; i < n; ++i) {
    if(arr[i] > mx)
    {
      mx=arr[i];
      p = i;
    }
  }
  printf("Max = %d at index %d\n", mx, p);
  return 0;
}
