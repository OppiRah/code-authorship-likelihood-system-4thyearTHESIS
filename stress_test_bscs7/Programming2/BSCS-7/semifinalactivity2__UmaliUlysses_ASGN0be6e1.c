#include <stdio.h>

int ls(int arr[],int n,int k) {
  for(int i=0; i<n; i++) {
    if(arr[i]==k) {
      return i;
    }
  }
  return -1;
}

int main() {
  int arr[]={5,17,3,42,8,29};
  int k;
  printf("Search for: ");
  scanf("%d",&k);
  int idx=ls(arr,6,k);
  if(idx>=0) {
    printf("Found at %d\n",idx);
  } else {
    printf("Not found\n");
  }
  return 0;
}
