#include <stdio.h>

float cavg(int arr[],int n);
int fmax(int arr[],int n);
int fmin(int arr[],int n);

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  for(int i=0; i<n; i++) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }
  printf("You entered: ");
  for(int i=0; i<n; i++) {
    printf("%d ",arr[i]);
  }
  printf("\n");
  int mx=fmax(arr,n);
  int mn=fmin(arr,n);
  float avg=cavg(arr,n);
  printf("Max: %d\nMin: %d\nAverage: %.2f\n",mx,mn,avg);
  return 0;
}

float cavg(int arr[],int n) {
  int s=0;
  for(int i=0; i<n; i++) {
    s=s+arr[i];
  }
  return (float)s/n;
}

int fmax(int arr[],int n) {
  int b=arr[0];
  for(int i=1; i<n; i++) {
    if(arr[i]>b) {
      b=arr[i];
    }
  }
  return b;
}

int fmin(int arr[],int n) {
  int b=arr[0];
  for(int i=1; i < n; i++) {
    if(arr[i]<b) {
      b=arr[i];
    }
  }
  return b;
}
