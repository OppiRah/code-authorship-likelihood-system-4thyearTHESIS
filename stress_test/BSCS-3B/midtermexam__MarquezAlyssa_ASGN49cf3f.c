// Seatwork: Array Statistics (Max/Min/Sum)
// Submitted by: Alyssa Marquez

#include <stdio.h>

int max(int a[], int n)
{
	int m = a[0];
	for (int i = 1; i < n; i++) if (a[i] > m) m = a[i];
	return m;
}

int min(int a[], int n)
{
	int m = a[0];
	for (int i = 1; i < n; i++) if (a[i] < m) m = a[i];
	return m;
}

int sum(int a[], int n)
{
	int s = 0;
	for (int i = 0; i < n; i++) s += a[i];
	return s;
}

int main()
{
	// compute max, min, sum using separate helper functions
	int arr[7] = { 42, 8, 23, 61, 4, 17, 33 };
	printf("Max: %d, Min: %d, Sum: %d\n", max(arr, 7), min(arr, 7), sum(arr, 7));
	return 0;
}
