#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

float ca(struct Student st[], int n)
{
  float t = 0;
  for(int i=0; i < n; i += 1) {
    t = t+st[i].grade;
  }
  return t/n;
}

int top(struct Student st[], int n) {
  int b = 0;
  for(int i = 1; i < n; i += 1) {
    if(st[i].grade > st[b].grade)
    {
      b = i;
    }
  }
  return b;
}

int main() {
  struct Student st[30];
  int n, ps = 0;
  scanf("%d", &n);
  if(n<=0 || n>50) {
    printf("Invalid size\n");
    return 1;
  }
  for(int i=0; i < n; i+=1)
  {
    scanf("%49s", st[i].name);
    scanf("%f", &st[i].grade);
  }
  for(int i = 0; i<n; i+=1)
  {
    if(st[i].grade >= 75)
    {
      ps+=1;
    }
  }
  printf("Class average: %.2f\n", ca(st, n));
  int b=top(st, n);
  printf("Top student: %s (%.2f)\n", st[b].name, st[b].grade);
  printf("Passed: %d, Failed: %d\n", ps, n-ps);

  int h=0;
  for(int i = 0; i < n; i += 1) {
    if(st[i].grade>=90)
    {
      h+=1;
    }
  }
  printf("With honors: %d\n", h);
  for(int i = 0; i < n; i+=1) {
    printf("%s: %s\n", st[i].name, st[i].grade>=75 ? "Passed" : "Failed");
  }
  return 0;
}
