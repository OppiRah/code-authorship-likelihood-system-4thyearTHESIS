#include <stdio.h>

int fmin(int arr[],int n);
float cavg(int arr[],int n);
int fmax(int arr[],int n);

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  if(n<=0||n>50) {
    printf("Invalid size\n");
    return 1;
  }
  for(int i=0; i<n; i+=1) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }
  int mx=fmax(arr,n);
  int mn = fmin(arr,n);
  float avg=cavg(arr,n);
  int ab=0;
  for(int i=0; i<n; i+=1) {
    if(arr[i] > avg) {
      ab+=1;
    }
  }
  printf("\n--- Results ---\n");
  printf("Max: %d\nMin: %d\nAverage: %.2f\nAbove average: %d\n",mx,mn,avg,ab);
  return 0;
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

float cavg(int arr[],int n) {
  int s=0;
  for(int i=0; i<n; i+=1) {
    s+=arr[i];
  }
  return s*1.0/n;
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
