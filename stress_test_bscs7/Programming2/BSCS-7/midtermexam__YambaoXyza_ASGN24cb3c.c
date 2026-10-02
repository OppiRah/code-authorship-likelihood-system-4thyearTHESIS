#include <stdio.h>
#include <string.h>
#include <ctype.h>

int gl(char s[])
{
  int c=0;
  while(s[c]!='\0') {
    c++;
  }
  return c;
}

void up(char s[]) {
  for(int i=0; s[i]!='\0'; i++)
  {
    if(s[i] >= 'a' && s[i]<='z') {
      s[i] = s[i] - 32;
    }
  }
}

void cv(char s[], int *v, int *k)
{
  *v = 0;
  *k=0;
  for(int i = 0; s[i]!='\0'; i++) {
    char c2 = tolower(s[i]);
    if(!isalpha(c2))
    {
      continue;
    }
    switch(c2) {
      case 'a': case 'e': case 'i': case 'o': case 'u':
      (*v)++;
      break;
      default:
      (*k)++;
    }
  }
}

int cpal(char s[]) {
  int l=0, r=gl(s) - 1;
  while(l < r)
  {
    if(tolower(s[l])!=tolower(s[r])) {
      return 0;
    }
    l++;
    r--;
  }
  return 1;
}

int main()
{
  char s[100];
  fgets(s, sizeof(s), stdin);
  s[strcspn(s, "\n")] = '\0';
  int v, k;
  cv(s, &v, &k);
  printf("Length: %d\n", gl(s));
  printf("Vowels: %d, Consonants: %d\n", v, k);
  printf("Palindrome: %s\n", cpal(s) ? "yes" : "no");
  up(s);
  printf("Uppercase: %s\n", s);
  int s2=0, idx=0;
  while(s[idx] != '\0') {
    if(s[idx]==' ') {
      s2+=1;
    }
    idx++;
  }
  printf("Spaces: %d\n", s2);
  if(strlen(s)>0)
  {
    printf("First: %c, Last: %c\n", s[0], s[strlen(s) - 1]);
  }
  return 0;
}
