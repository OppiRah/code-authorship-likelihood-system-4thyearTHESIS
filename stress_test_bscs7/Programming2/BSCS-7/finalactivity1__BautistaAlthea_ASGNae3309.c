#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

void ps(struct Student st) {
  printf("Name: %s\n", st.name);
  printf("Grade: %.2f\n", st.grade);
}

int main() {
  struct Student st;
  printf("Name: ");
  scanf("%49s", st.name);
  printf("Grade: ");
  scanf("%f", &st.grade);
  ps(st);
  return 0;
}
