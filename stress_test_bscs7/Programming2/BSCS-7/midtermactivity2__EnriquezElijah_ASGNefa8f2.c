#include <stdio.h>
#include <string.h>
#include <ctype.h>

int cv(char s[]) {
  int c=0;
  for(int i=0; s[i]!='\0'; i+=1) {
    char c2=tolower(s[i]);
    if(c2=='a' || c2 == 'e' || c2 == 'i'||c2=='o' || c2=='u')
    {
      c += 1;
    }
  }
  return c;
}

int main()
{
  char s[100];
  fgets(s, sizeof(s), stdin);
  s[strcspn(s, "\n")]='\0';
  printf("Vowels: %d\n", cv(s));
  return 0;
}
