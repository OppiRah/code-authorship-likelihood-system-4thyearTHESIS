#include <stdio.h>
#include <string.h>
#include <ctype.h>

void cv(char s[], int *v, int *k) {
  *v=0;
  *k=0;
  for(int i = 0; s[i] != '\0'; ++i)
  {
    char c=tolower(s[i]);
    if(!isalpha(c))
    {
      continue;
    }
    if(c=='a'||c=='e' || c=='i' || c=='o' || c == 'u') {
      *v=*v+1;
    }
    else
    {
      *k=*k+1;
    }
  }
}

void up(char s[])
{
  for(int i=0; s[i] != '\0'; ++i)
  {
    s[i]=toupper(s[i]);
  }
}

int cpal(char s[]) {
  char cp[100];
  int n=strlen(s);
  for(int i = 0; i < n; ++i)
  {
    cp[i] = tolower(s[n-1 - i]);
  }
  cp[n]='\0';
  for(int i = 0; i < n; ++i) {
    if(tolower(s[i]) != cp[i])
    {
      return 0;
    }
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
  printf("Length: %d\n", strlen(s));
  printf("Vowels: %d, Consonants: %d\n", v, k);
  printf("Palindrome: %s\n", cpal(s) ? "yes" : "no");
  up(s);
  printf("Uppercase: %s\n", s);

  int dc = 0;
  for(int i=0; s[i] != '\0'; ++i) {
    if(isdigit(s[i]))
    {
      ++dc;
    }
  }
  printf("Digits: %d\n", dc);
  return 0;
}
