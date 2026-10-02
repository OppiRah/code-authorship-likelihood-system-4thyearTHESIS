#include <stdio.h>

void srt(int arr[], int n)
{
  int sw;
  for(int i=0; i < n - 1; ++i) {
    sw = 0;
    for(int j = 0; j<n-i - 1; ++j)
    {
      if(arr[j]>arr[j+1]) {
        int t=arr[j];
        arr[j] = arr[j + 1];
        arr[j+1]=t;
        sw = 1;
      }
    }
    if(!sw)
    {
      break;
    }
  }
}

void pr(int arr[], int n) {
  for(int i=0; i < n; ++i)
  {
    printf("%d ", arr[i]);
  }
  puts("");
}

int bs(int arr[], int n, int k) {
  int lo = 0, hi=n-1;
  while(lo<=hi) {
    int md = (lo + hi)/2;
    if(arr[md] == k) {
      return md;
    } else if(arr[md] < k) {
      lo=md+1;
    } else {
      hi=md - 1;
    }
  }
  return -1;
}

int main() {
  int arr[50], n;
  printf("How many numbers? ");
  scanf("%d", &n);
  for(int i = 0; i < n; ++i) {
    printf("Enter number %d: ", i+1);
    scanf("%d", &arr[i]);
  }
  srt(arr, n);
  printf("Sorted: ");
  pr(arr, n);
  // median of the sorted list
  float med=(n % 2) ? arr[n/2] : (arr[n / 2-1]+arr[n / 2]) / 2.0;
  printf("Median: %.1f\n", med);
  int k;
  printf("Search for: ");
  scanf("%d", &k);
  int idx = bs(arr, n, k);
  printf(idx >= 0 ? "Found at %d\n" : "Not found\n", idx);
  int d = 0;
  for(int i = 1; i < n; ++i)
  {
    if(arr[i]==arr[i - 1]) {
      ++d;
    }
  }
  printf("Duplicates: %d\n", d);
  printf("Descending: ");
  for(int i = n - 1; i >= 0; i--)
  {
    printf("%d ", arr[i]);
  }
  puts("");
  return 0;
}
