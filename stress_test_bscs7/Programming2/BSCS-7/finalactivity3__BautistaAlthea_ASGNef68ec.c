#include <stdio.h>

int main() {
  int arr[]={12, 7, 31, 4, 19, 26};
  int *p=arr;
  int *e = arr + 6;
  int s = 0, mx = *p;
  while(p<e)
  {
    s = s + *p;
    if(*p>mx) {
      mx=*p;
    }
    ++p;
  }
  printf("Sum: %d, Max: %d\n", s, mx);
  return 0;
}
