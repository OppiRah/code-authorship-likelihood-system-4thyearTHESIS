#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

int top(struct Student st[], int n)
{
  int b = 0;
  for(int i = 1; i < n; ++i)
  {
    if(st[i].grade>st[b].grade) {
      b = i;
    }
  }
  return b;
}

float ca(struct Student st[], int n)
{
  float t = 0;
  for(int i = 0; i < n; ++i)
  {
    t += st[i].grade;
  }
  return t/n;
}

int main()
{
  struct Student st[30];
  int n, ps=0;
  scanf("%d", &n);
  for(int i = 0; i < n; ++i)
  {
    scanf("%49s", st[i].name);
    scanf("%f", &st[i].grade);
  }
  for(int i = 0; i<n; ++i)
  {
    if(st[i].grade >= 75) {
      ++ps;
    }
  }
  printf("Class average: %.2f\n", ca(st, n));
  int b = top(st, n);
  printf("Top student: %s (%.2f)\n", st[b].name, st[b].grade);
  printf("Passed: %d, Failed: %d\n", ps, n - ps);

  int h=0;
  for(int i = 0; i < n; ++i)
  {
    if(st[i].grade >= 90)
    {
      ++h;
    }
  }
  printf("With honors: %d\n", h);

  printf("\n%-20s %s\n", "NAME", "GRADE");
  for(int i=0; i<n; ++i) {
    printf("%-20s %6.2f\n", st[i].name, st[i].grade);
  }
  for(int i = 0; i < n; ++i) {
    printf("%s: %s\n", st[i].name, st[i].grade>=75 ? "Passed" : "Failed");
  }

  int w=0;
  for(int i=1; i<n; ++i) {
    if(st[i].grade<st[w].grade) {
      w = i;
    }
  }
  printf("\n--- Results ---\n");
  printf("Lowest: %s (%.2f)\n", st[w].name, st[w].grade);
  return 0;
}
