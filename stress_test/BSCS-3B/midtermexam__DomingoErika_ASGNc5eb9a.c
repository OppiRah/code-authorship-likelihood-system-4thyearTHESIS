// Class activity: Array Statistics (Max/Min/Sum)
// Submitted by: Erika Domingo

#include <stdio.h>

int maxvalue(int a[], int n)
{
  int m = a[0];
  for (int i = 1; i < n; i++) if (a[i] > m) m = a[i];
  return m;
}

int minvalue(int a[], int n)
{
  int m = a[0];
  for (int i = 1; i < n; i++) if (a[i] < m) m = a[i];
  return m;
}

int sumvalue(int a[], int n)
{
  int s = 0;
  for (int i = 0; i < n; i++) s += a[i];
  return s;
}

int main()
{
  // compute max, min, sum using separate helper functions
  int arr[7] = { 42, 8, 23, 61, 4, 17, 33 };
  printf("Max: %d, Min: %d, Sum: %d\n", maxvalue(arr, 7), minvalue(arr, 7), sumvalue(arr, 7));
  return 0;
}
