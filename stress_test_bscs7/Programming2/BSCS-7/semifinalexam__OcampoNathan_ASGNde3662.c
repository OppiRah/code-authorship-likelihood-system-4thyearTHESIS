#include <stdio.h>

int bs(int arr[],int n,int k);
void pr(int arr[],int n);
void srt(int arr[],int n);

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  for(int i=0; i<n; i+=1) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }
  srt(arr,n);
  printf("Sorted: ");
  pr(arr,n);
  float med;
  if(n%2==0) {
    med = (arr[n/2-1]+arr[n/2])/2.0;
  } else {
    med=arr[n/2];
  }
  printf("Median: %.1f\n",med);
  int k;
  printf("Search for: ");
  scanf("%d",&k);
  int idx=bs(arr,n,k);
  printf(idx>=0 ? "Found at %d\n" : "Not found\n",idx);

  int t=0;
  for(int i=0; i<n; i+=1) {
    t+=arr[i];
  }
  printf("Sum: %d, Average: %.2f\n",t,(float)t/n);
  int d=0;
  for(int i=1; i < n; i+=1) {
    if(arr[i]==arr[i-1]) {
      d+=1;
    }
  }
  printf("Duplicates: %d\n",d);
  printf("Descending: ");
  for(int i = n-1; i>=0; i--) {
    printf("%d ",arr[i]);
  }
  printf("\n");
  return 0;
}

int bs(int arr[],int n,int k) {
  for(int i=0; i<n&&arr[i]<=k; i+=1) {
    if(arr[i]==k) {
      return i;
    }
  }
  return -1;
}

void pr(int arr[],int n) {
  for(int i = 0; i<n; i+=1) {
    printf("%d ",arr[i]);
  }
  printf("\n");
}

void srt(int arr[],int n) {
  for(int i = 0; i<n-1; i+=1) {
    int mi=i;
    for(int j=i+1; j<n; j+=1) {
      if(arr[j]<arr[mi]) {
        mi=j;
      }
    }
    int t2=arr[i];
    arr[i]=arr[mi];
    arr[mi]=t2;
  }
}
