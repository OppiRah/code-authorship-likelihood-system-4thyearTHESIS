#include <stdio.h>
#include <string.h>

void rv(char s[]) {
  int l = 0, r = strlen(s)-1;
  while(l<r) {
    char t=s[l];
    s[l] = s[r];
    s[r]=t;
    l+=1;
    r--;
  }
}

int main()
{
  char s[100];
  fgets(s, sizeof(s), stdin);
  // drop the newline
  s[strcspn(s, "\n")]='\0';
  rv(s);
  printf("Reversed: %s\n", s);
  return 0;
}
