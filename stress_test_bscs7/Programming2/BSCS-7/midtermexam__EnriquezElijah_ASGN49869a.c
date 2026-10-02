#include <stdio.h>
#include <string.h>
#include <ctype.h>

int cpal(char s[]);
void up(char s[]);
void cv(char s[], int *v, int *k);
int gl(char s[]);

int main() {
  char s[100];
  fgets(s, sizeof(s), stdin);
  // drop the newline
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
      s2 += 1;
    }
    idx+=1;
  }
  printf("Spaces: %d\n", s2);
  if(strlen(s)>0) {
    printf("First: %c, Last: %c\n", s[0], s[strlen(s)-1]);
  }
  return 0;
}

int cpal(char s[]) {
  char cp[100];
  int n = gl(s);
  for(int i = 0; i<n; i += 1) {
    cp[i] = tolower(s[n - 1-i]);
  }
  cp[n]='\0';
  for(int i = 0; i<n; i+=1) {
    if(tolower(s[i])!=cp[i]) {
      return 0;
    }
  }
  return 1;
}

void up(char s[]) {
  for(int i = 0; s[i] != '\0'; i+=1)
  {
    s[i]=toupper(s[i]);
  }
}

void cv(char s[], int *v, int *k) {
  *v=0;
  *k = 0;
  for(int i=0; s[i] != '\0'; i += 1) {
    char c = tolower(s[i]);
    if(!isalpha(c))
    {
      continue;
    }
    if(c == 'a' || c=='e' || c=='i'||c == 'o' || c=='u')
    {
      *v=*v + 1;
    } else {
      *k=*k+1;
    }
  }
}

int gl(char s[]) {
  int c2 = 0;
  while(s[c2]!='\0')
  {
    c2+=1;
  }
  return c2;
}
