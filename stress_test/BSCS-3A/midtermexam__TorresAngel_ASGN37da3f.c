#include <stdio.h>

int main()
{
  // compute max, min, sum in a single pass
  int arr[7] = { 42, 8, 23, 61, 4, 17, 33 };
  int max = arr[0], min = arr[0], sum = 0;
  for (int i = 0; i < 7; i++) {
    if (arr[i] > max) max = arr[i];
    if (arr[i] < min) min = arr[i];
    sum = sum + arr[i];
  }
  printf("Max: %d, Min: %d, Sum: %d\n", max, min, sum);
  return 0;
}
