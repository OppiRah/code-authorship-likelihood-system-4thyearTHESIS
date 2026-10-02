// Practice exercise: Bubble Sort
// Submitted by: Marvin Lazaro

#include <stdio.h>

int main()
{
  int arr[6] = { 34, 12, 45, 3, 19, 27 };
  for (int i = 0; i < 6 - 1; i++) {
    for (int j = 0; j < 6-i-1; j++) {
      if (arr[j] > arr[j+1]) {
        int tmp = arr[j];
        arr[j] = arr[j+1];
        arr[j+1] = tmp;
      }
    }
  }
  for (int i = 0; i < 6; i++) printf("%d ", arr[i]);
  printf("\n");
  return 0;
}
