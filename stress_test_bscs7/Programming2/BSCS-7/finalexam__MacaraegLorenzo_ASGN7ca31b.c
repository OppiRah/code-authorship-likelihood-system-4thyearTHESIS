#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

int top(struct Student st[],int n);
float ca(struct Student st[],int n);

int main() {
  struct Student st[30];
  int n,ps=0;
  printf("Number of students: ");
  scanf("%d",&n);
  for(int i=0; i<n; i+=1) {
    printf("Name of student %d: ",i+1);
    scanf("%49s",st[i].name);
    printf("Grade: ");
    scanf("%f",&st[i].grade);
    if(st[i].grade>=75) {
      ps+=1;
    }
  }
  printf("Class average: %.2f\n",ca(st,n));
  int b=top(st,n);
  printf("Top student: %s (%.2f)\n",st[b].name,st[b].grade);
  printf("Passed: %d, Failed: %d\n",ps,n-ps);
  int w=0;
  for(int i=1; i<n; i+=1) {
    if(st[i].grade<st[w].grade) {
      w=i;
    }
  }
  printf("Lowest: %s (%.2f)\n",st[w].name,st[w].grade);
  for(int i=0; i < n; i+=1) {
    printf("%s: %s\n",st[i].name,st[i].grade>=75 ? "Passed" : "Failed");
  }
  return 0;
}

int top(struct Student st[],int n) {
  int b=0;
  for(int i=1; i<n; i+=1) {
    if(st[i].grade>st[b].grade) {
      b=i;
    }
  }
  return b;
}

float ca(struct Student st[],int n) {
  float t=0;
  for(int i=0; i<n; i+=1) {
    t=t+st[i].grade;
  }
  return t/n;
}
