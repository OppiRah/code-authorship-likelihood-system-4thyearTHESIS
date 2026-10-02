#include <stdio.h>

float cavg(int arr[],int n);
int fmax(int arr[],int n);
int fmin(int arr[],int n);

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  for(int i=0; i<n; i+=1) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }

  int mx=fmax(arr,n);
  int mn=fmin(arr,n);
  float avg=cavg(arr,n);
  printf("Max: %d\nMin: %d\nAverage: %.2f\n",mx,mn,avg);
  return 0;
}

float cavg(int arr[],int n) {
  int s=0;
  for(int i=0; i<n; i+=1) {
    s+=arr[i];
  }
  return (float)s/n;
}

int fmax(int arr[],int n) {
  int b=arr[0];
  for(int i=1; i<n; i+=1) {
    if(arr[i]>b) {
      b=arr[i];
    }
  }
  return b;
}

int fmin(int arr[],int n) {
  int b=arr[0];
  for(int i=1; i<n; i+=1) {
    if(arr[i]<b) {
      b=arr[i];
    }
  }
  return b;
}
