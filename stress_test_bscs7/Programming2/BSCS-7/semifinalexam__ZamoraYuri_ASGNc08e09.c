#include <stdio.h>

void srt(int arr[], int n);
int bs(int arr[], int n, int k);
void pr(int arr[], int n);

int main()
{
  int arr[50], n;
  scanf("%d", &n);
  for(int i=0; i<n; ++i)
  {
    scanf("%d", &arr[i]);
  }
  srt(arr, n);
  printf("Sorted: ");
  pr(arr, n);
  // median of the sorted list
  float med=(n % 2) ? arr[n / 2] : (arr[n/2 - 1] + arr[n/2])/2.0;
  printf("Median: %.1f\n", med);
  int k;
  scanf("%d", &k);
  int idx=bs(arr, n, k);
  printf(idx>=0 ? "Found at %d\n" : "Not found\n", idx);

  printf("Descending: ");
  for(int i = n - 1; i >= 0; i--) {
    printf("%d ", arr[i]);
  }
  printf("\n");

  int d = 0;
  for(int i=1; i<n; ++i) {
    if(arr[i] == arr[i - 1])
    {
      ++d;
    }
  }
  printf("Duplicates: %d\n", d);
  printf("\n--- Results ---\n");
  printf("Smallest: %d, Largest: %d\n", arr[0], arr[n-1]);
  return 0;
}

void srt(int arr[], int n) {
  for(int i = 0; i < n-1; ++i)
  {
    for(int j = 0; j<n - i-1; ++j) {
      if(arr[j] > arr[j + 1]) {
        int t=arr[j];
        arr[j] = arr[j+1];
        arr[j+1] = t;
      }
    }
  }
}

int bs(int arr[], int n, int k) {
  for(int i=0; i < n && arr[i]<=k; ++i)
  {
    if(arr[i] == k)
    {
      return i;
    }
  }
  return -1;
}

void pr(int arr[], int n)
{
  for(int i=0; i<n; ++i) {
    printf("%d ", arr[i]);
  }
  printf("\n");
}
