#include <stdio.h>

void pr(int arr[], int n);
void srt(int arr[], int n);
int bs(int arr[], int n, int k);

int main() {
  int arr[50], n;
  scanf("%d", &n);
  if(n <= 0||n>50) {
    printf("Invalid size\n");
    return 1;
  }
  for(int i = 0; i<n; i+=1)
  {
    scanf("%d", &arr[i]);
  }
  printf("You entered: ");
  for(int i=0; i<n; i += 1)
  {
    printf("%d ", arr[i]);
  }
  printf("\n");
  srt(arr, n);
  printf("Sorted: ");
  pr(arr, n);
  float med;
  if(n % 2==0)
  {
    med=(arr[n/2-1]+arr[n/2])/2.0;
  }
  else
  {
    med=arr[n / 2];
  }
  printf("Median: %.1f\n", med);
  int k;
  scanf("%d", &k);
  int idx=bs(arr, n, k);
  printf(idx >= 0 ? "Found at %d\n" : "Not found\n", idx);

  printf("Smallest: %d, Largest: %d\n", arr[0], arr[n - 1]);
  return 0;
}

void pr(int arr[], int n) {
  for(int i = 0; i<n; i+=1)
  {
    printf("%d ", arr[i]);
  }
  printf("\n");
}

void srt(int arr[], int n) {
  for(int i = 0; i<n-1; i+=1) {
    int mi=i;
    for(int j=i+1; j < n; j += 1)
    {
      if(arr[j]<arr[mi]) {
        mi = j;
      }
    }
    int t=arr[i];
    arr[i]=arr[mi];
    arr[mi] = t;
  }
}

int bs(int arr[], int n, int k) {
  int lo = 0, hi=n-1;
  while(lo<=hi) {
    int md=(lo+hi) / 2;
    if(arr[md]==k)
    {
      return md;
    } else if(arr[md] < k) {
      lo = md+1;
    }
    else
    {
      hi = md - 1;
    }
  }
  return -1;
}
