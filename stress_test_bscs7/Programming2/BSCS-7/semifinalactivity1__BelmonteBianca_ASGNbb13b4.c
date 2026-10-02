#include <stdio.h>

void pr(int arr[],int n) {
  for(int i=0; i<n; i++) {
    printf("%d ",arr[i]);
  }
  printf("\n");
}

void srt(int arr[],int n) {
  int sw;
  for(int i=0; i<n-1; i++) {
    sw=0;
    for(int j=0; j<n-i-1; j++) {
      if(arr[j]>arr[j+1]) {
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

int main() {
  int arr[]={64,25,12,22,11,90,38};
  int n = 7;
  printf("Before: ");
  pr(arr,n);
  srt(arr,n);
  printf("After: ");
  pr(arr,n);
  return 0;
}
