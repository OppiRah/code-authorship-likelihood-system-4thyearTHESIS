#include <stdio.h>

int bs(int arr[],int n,int k);
void pr(int arr[],int n);
void srt(int arr[],int n);

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  for(int i=0; i<n; ++i) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }
  srt(arr,n);
  printf("Sorted: ");
  pr(arr,n);
  float med=(n%2) ? arr[n/2] : (arr[n/2-1]+arr[n/2])/2.0;
  printf("Median: %.1f\n",med);
  int k;
  printf("Search for: ");
  scanf("%d",&k);
  int idx=bs(arr,n,k);
  printf(idx>=0 ? "Found at %d\n" : "Not found\n",idx);
  int t=0;
  for(int i=0; i<n; ++i) {
    t=t+arr[i];
  }
  printf("Sum: %d, Average: %.2f\n",t,t*1.0/n);
  return 0;
}

int bs(int arr[],int n,int k) {
  for(int i=0; i<n&&arr[i]<=k; ++i) {
    if(arr[i]==k) {
      return i;
    }
  }
  return -1;
}

void pr(int arr[],int n) {
  for(int i=0; i<n; ++i) {
    printf("%d ",arr[i]);
  }
  printf("\n");
}

void srt(int arr[],int n) {
  for(int i=0; i<n-1; ++i) {
    for(int j=0; j<n-i-1; ++j) {
      if(arr[j]>arr[j+1]) {
        int t2 = arr[j];
        arr[j]=arr[j+1];
        arr[j+1]=t2;
      }
    }
  }
}
