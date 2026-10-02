#include <stdio.h>
#include <string.h>
#include <ctype.h>

int cpal(char s[]) {
  int l=0, r = strlen(s) - 1;
  while(l < r)
  {
    if(tolower(s[l])!=tolower(s[r]))
    {
      return 0;
    }
    ++l;
    r--;
  }
  return 1;
}

void cv(char s[], int *v, int *k)
{
  *v = 0;
  *k = 0;
  for(int i = 0; s[i] != '\0'; ++i) {
    char c=tolower(s[i]);
    if(!isalpha(c))
    {
      continue;
    }
    if(c=='a'||c=='e' || c == 'i' || c=='o'||c == 'u')
    {
      *v = *v + 1;
    } else {
      *k = *k + 1;
    }
  }
}

void up(char s[]) {
  for(int i=0; s[i]!='\0'; ++i) {
    if(s[i] >= 'a'&&s[i] <= 'z') {
      s[i]=s[i]-32;
    }
  }
}

int main() {
  char s[100];
  printf("Enter a string: ");
  fgets(s, sizeof(s), stdin);
  s[strcspn(s, "\n")]='\0';
  int v, k;
  cv(s, &v, &k);
  printf("Length: %d\n", strlen(s));
  printf("Vowels: %d, Consonants: %d\n", v, k);
  if(cpal(s))
  {
    puts("It is a palindrome");
  }
  else
  {
    puts("Not a palindrome");
  }
  up(s);
  printf("Uppercase: %s\n", s);
  int w=0, iw = 0;
  for(int i = 0; s[i] != '\0'; ++i)
  {
    if(s[i]==' ')
    {
      iw = 0;
    } else if(!iw) {
      iw = 1;
      ++w;
    }
  }
  printf("Words: %d\n", w);
  if(strlen(s)>0) {
    printf("First: %c, Last: %c\n", s[0], s[strlen(s)-1]);
  }
  return 0;
}
