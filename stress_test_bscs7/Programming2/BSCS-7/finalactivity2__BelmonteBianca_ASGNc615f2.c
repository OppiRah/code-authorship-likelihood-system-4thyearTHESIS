#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

float ca(struct Student st[],int n) {
  float t=0;
  for(int i=0; i<n; i++) {
    t=t+st[i].grade;
  }
  return t/n;
}

int main() {
  struct Student st[3]={{"Ana",88},{"Ben",74},{"Cara",91}};
  printf("Class average: %.2f\n",ca(st,3));
  return 0;
}
