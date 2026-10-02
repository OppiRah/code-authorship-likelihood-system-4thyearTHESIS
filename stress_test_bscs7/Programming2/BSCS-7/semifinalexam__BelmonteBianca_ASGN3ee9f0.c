#include <stdio.h>

int bs(int arr[],int n,int k);
void pr(int arr[],int n);
void srt(int arr[],int n);

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  for(int i=0; i<n; i++) {
    printf("Enter number %d: ",i+1);
    scanf("%d",&arr[i]);
  }
  srt(arr,n);
  printf("Sorted: ");
  pr(arr,n);
  float med;
  if(n%2==0) {
    med=(arr[n/2-1]+arr[n/2])/2.0;
  } else {
    med=arr[n/2];
  }
  printf("Median: %.1f\n",med);
  int k;
  printf("Search for: ");
  scanf("%d",&k);
  int idx = bs(arr,n,k);
  printf(idx>=0 ? "Found at %d\n" : "Not found\n",idx);
  printf("Smallest: %d, Largest: %d\n",arr[0],arr[n-1]);
  int t=0;
  for(int i=0; i<n; i++) {
    t=t+arr[i];
  }
  printf("\n--- Results ---\n");
  printf("Sum: %d, Average: %.2f\n",t,(float)t/n);
  return 0;
}

int bs(int arr[],int n,int k) {
  int lo=0,hi = n-1;
  while(lo<=hi) {
    int md=(lo+hi)/2;
    if(arr[md]==k) {
      return md;
    } else if(arr[md]<k) {
      lo=md+1;
    } else {
      hi=md-1;
    }
  }
  return -1;
}

void pr(int arr[],int n) {
  for(int i=0; i<n; i++) {
    printf("%d ",arr[i]);
  }
  printf("\n");
}

void srt(int arr[],int n) {
  for(int i=0; i<n-1; i++) {
    int mi=i;
    for(int j=i+1; j<n; j++) {
      if(arr[j]<arr[mi]) {
        mi=j;
      }
    }
    int t2=arr[i];
    arr[i]=arr[mi];
    arr[mi]=t2;
  }
}
