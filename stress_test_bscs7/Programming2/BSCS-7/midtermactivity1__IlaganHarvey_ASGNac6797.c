#include <stdio.h>
#include <string.h>

int gl(char s[]) {
  int c=0;
  while(s[c]!='\0') {
    c++;
  }
  return c;
}

int main() {
  char s[100];
  printf("Enter a string: ");
  fgets(s,sizeof(s),stdin);
  s[strcspn(s,"\n")]='\0';
  printf("Length: %d\n",gl(s));
  return 0;
}
