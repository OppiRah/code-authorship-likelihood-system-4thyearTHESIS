#include <stdio.h>

void srt(int arr[],int n) {
  for(int i=0; i<n-1; i++) {
    int mi=i;
    for(int j=i+1; j<n; j++) {
      if(arr[j]<arr[mi]) {
        mi=j;
      }
    }
    int t=arr[i];
    arr[i]=arr[mi];
    arr[mi]=t;
  }
}

int bs(int arr[],int n,int k) {
  int lo=0,hi=n-1;
  while(lo<=hi) {
    int md=(lo+hi)/2;
    if(arr[md]==k) {
      return md;
    } else if(arr[md]<k) {
      lo = md+1;
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
  srt(arr,n);
  printf("Sorted: ");
  pr(arr,n);
  float med=(n%2) ? arr[n / 2] : (arr[n/2-1]+arr[n/2])/2.0;
  printf("Median: %.1f\n",med);
  int k;
  printf("Search for: ");
  scanf("%d",&k);
  int idx = bs(arr,n,k);
  printf(idx >= 0 ? "Found at %d\n" : "Not found\n",idx);
  int t2=0;
  for(int i=0; i<n; i++) {
    t2+=arr[i];
  }
  printf("Sum: %d, Average: %.2f\n",t2,(float)t2 / n);
  int d = 0;
  for(int i=1; i < n; i++) {
    if(arr[i]==arr[i-1]) {
      d++;
    }
  }
  printf("Duplicates: %d\n",d);
  return 0;
}
