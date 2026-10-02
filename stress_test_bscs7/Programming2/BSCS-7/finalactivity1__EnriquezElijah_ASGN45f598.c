#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

void ps(struct Student st)
{
  printf("Name: %s\n", st.name);
}

int main() {
  struct Student st;
  printf("Name: ");
  scanf("%49s", st.name);
  scanf("%f", &st.grade);
  ps(st);
  return 0;
}
