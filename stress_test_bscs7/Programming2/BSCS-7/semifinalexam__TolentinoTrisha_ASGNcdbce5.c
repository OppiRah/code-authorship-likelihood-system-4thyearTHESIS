#include <stdio.h>

void pr(int arr[],int n);
int binary_search(int arr[],int n,int k);
void sort_array(int arr[],int n);

int main() {
  int arr[50],n;
  scanf("%d",&n);
  if(n<=0||n > 50) {
    printf("Invalid size\n");
    return 1;
  }
  for(int i=0; i<n; i++) {
    scanf("%d",&arr[i]);
  }
  sort_array(arr,n);
  printf("Sorted: ");
  pr(arr,n);
  float med;
  if(n%2==0) {
    med=(arr[n/2 - 1]+arr[n/2])/2.0;
  } else {
    med=arr[n/2];
  }
  printf("Median: %.1f\n",med);
  int k;
  scanf("%d",&k);
  int idx = binary_search(arr,n,k);
  printf(idx>=0 ? "Found at %d\n" : "Not found\n",idx);

  printf("\n--- Results ---\n");
  printf("Smallest: %d, Largest: %d\n",arr[0],arr[n-1]);
  return 0;
}

void pr(int arr[],int n) {
  for(int i=0; i<n; i++) {
    printf("%d ",arr[i]);
  }
  printf("\n");
}

int binary_search(int numbers[], int size, int key) {
    int low = 0, high = size - 1;
    while (low <= high) {
        int mid = (low + high) / 2;  // middle index
        if (numbers[mid] == key) {
            return mid;
        } else if (numbers[mid] < key) {
            low=mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

void sort_array(int numbers[], int size) {
    int swapped;
    for (int i=0; i < size - 1; ++i) {
        swapped = 0;
        for (int j = 0; j < size - i - 1; ++j) {
            if (numbers[j] > numbers[j + 1]) {
                int temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
                swapped = 1;
            }
        }
        if (!swapped) {
            break;  // already sorted
        }
    }
}
