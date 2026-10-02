#include <stdio.h>

int main() {
  int m[3][3],t=0;
  for(int i=0; i<3; i+=1) {
    for(int j=0; j<3; j += 1) {
      printf("m[%d][%d]: ",i,j);
      scanf("%d",&m[i][j]);
    }
  }
  for(int i=0; i<3; i+=1) {
    int rs=0;
    for(int j=0; j<3; j+=1) {
      rs+=m[i][j];
    }
    printf("Row %d sum: %d\n",i,rs);
    t+=rs;
  }
  printf("Total: %d\n",t);
  return 0;
}
