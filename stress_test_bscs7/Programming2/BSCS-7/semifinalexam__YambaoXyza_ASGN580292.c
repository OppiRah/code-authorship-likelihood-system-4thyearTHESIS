#include <stdio.h>

int bs(int arr[], int n, int k) {
  for(int i = 0; i<n && arr[i]<=k; i++)
  {
    if(arr[i] == k)
    {
      return i;
    }
  }
  return -1;
}

void srt(int arr[], int n)
{
  for(int i=0; i < n - 1; i++) {
    int mi = i;
    for(int j=i+1; j<n; j++)
    {
      if(arr[j] < arr[mi]) {
        mi=j;
      }
    }
    int t = arr[i];
    arr[i]=arr[mi];
    arr[mi]=t;
  }
}

void pr(int arr[], int n) {
  for(int i = 0; i < n; i++)
  {
    printf("%d ", arr[i]);
  }
  printf("\n");
}

int main() {
  int arr[50], n;
  scanf("%d", &n);
  for(int i=0; i < n; i++) {
    scanf("%d", &arr[i]);
  }
  printf("You entered: ");
  for(int i = 0; i<n; i++)
  {
    printf("%d ", arr[i]);
  }
  printf("\n");
  srt(arr, n);
  printf("Sorted: ");
  pr(arr, n);
  float med;
  if(n%2 == 0)
  {
    med = (arr[n/2 - 1]+arr[n/2]) / 2.0;
  }
  else
  {
    med=arr[n/2];
  }
  printf("Median: %.1f\n", med);
  int k;
  scanf("%d", &k);
  int idx=bs(arr, n, k);
  printf(idx >= 0 ? "Found at %d\n" : "Not found\n", idx);
  printf("Descending: ");
  for(int i=n-1; i >= 0; i--)
  {
    printf("%d ", arr[i]);
  }
  printf("\n");
  printf("Smallest: %d, Largest: %d\n", arr[0], arr[n-1]);

  int d=0;
  for(int i = 1; i < n; i++)
  {
    if(arr[i] == arr[i-1]) {
      d++;
    }
  }
  printf("Duplicates: %d\n", d);
  int t2=0;
  for(int i = 0; i<n; i++)
  {
    t2 += arr[i];
  }
  printf("\n--- Results ---\n");
  printf("Sum: %d, Average: %.2f\n", t2, (float)t2/n);
  return 0;
}
