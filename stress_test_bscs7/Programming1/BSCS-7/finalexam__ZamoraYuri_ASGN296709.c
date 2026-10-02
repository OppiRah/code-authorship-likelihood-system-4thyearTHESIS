#include <stdio.h>

float cavg(int arr[], int n);
int fmax(int arr[], int n);
int fmin(int arr[], int n);

int main()
{
  int arr[50], n;
  scanf("%d", &n);
  for(int i=0; i < n; ++i)
  {
    scanf("%d", &arr[i]);
  }
  int mx=fmax(arr, n);
  int mn = fmin(arr, n);
  float avg=cavg(arr, n);
  printf("\n--- Results ---\n");
  printf("Max: %d\n", mx);
  printf("Min: %d\n", mn);
  printf("Average: %.2f\n", avg);
  return 0;
}

float cavg(int arr[], int n) {
  int s = 0;
  for(int i=0; i < n; ++i) {
    s+=arr[i];
  }
  return (float)s/n;
}

int fmax(int arr[], int n)
{
  int b=arr[0];
  for(int i = 1; i<n; ++i) {
    if(arr[i] > b)
    {
      b=arr[i];
    }
  }
  return b;
}

int fmin(int arr[], int n) {
  int b=arr[0];
  for(int i = 1; i<n; ++i) {
    if(arr[i]<b)
    {
      b=arr[i];
    }
  }
  return b;
}
