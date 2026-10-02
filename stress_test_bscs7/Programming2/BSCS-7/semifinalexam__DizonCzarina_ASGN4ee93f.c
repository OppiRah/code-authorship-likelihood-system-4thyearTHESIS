#include <stdio.h>

void pr(int arr[],int n);
void srt(int arr[],int n);
int bs(int arr[],int n,int k);

int main() {
  int arr[50],n;
  printf("How many numbers? ");
  scanf("%d",&n);
  if(n<=0||n>50) {
    puts("Invalid size");
    return 1;
  }
  for(int i=0; i<n; i+=1) {
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
  printf("\n--- Results ---\n");
  printf("Smallest: %d, Largest: %d\n",arr[0],arr[n-1]);
  return 0;
}

void pr(int arr[],int n) {
  for(int i=0; i<n; i+=1) {
    printf("%d ",arr[i]);
  }
  puts("");
}

void srt(int arr[],int n) {
  int sw;
  for(int i=0; i<n-1; i+=1) {
    sw=0;
    for(int j=0; j<n-i-1; j+=1) {
      if(arr[j]>arr[j + 1]) {
        int t=arr[j];
        arr[j]=arr[j+1];
        arr[j+1]=t;
        sw=1;
      }
    }
    if(!sw) {
      break;
    }
  }
}

int bs(int arr[],int n,int k) {
  for(int i = 0; i<n&&arr[i]<=k; i+=1) {
    if(arr[i]==k) {
      return i;
    }
  }
  return -1;
}
