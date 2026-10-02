#include <stdio.h>

int main()
{
  int m[3][3], t = 0;
  for(int i = 0; i<3; ++i)
  {
    for(int j = 0; j<3; ++j)
    {
      printf("m[%d][%d]: ", i, j);
      scanf("%d", &m[i][j]);
    }
  }
  for(int i=0; i<3; ++i)
  {
    int rs = 0;
    for(int j = 0; j<3; ++j) {
      rs=rs + m[i][j];
    }
    printf("Row %d sum: %d\n", i, rs);
    t = t + rs;
  }
  printf("Total: %d\n", t);
  return 0;
}
