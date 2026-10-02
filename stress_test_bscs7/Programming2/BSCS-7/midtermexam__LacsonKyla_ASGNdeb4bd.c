#include <stdio.h>
#include <string.h>
#include <ctype.h>

void up(char s[]);
int cpal(char s[]);
void cv(char s[],int *v,int *k);

int main() {
  char s[100];
  printf("Enter a string: ");
  fgets(s,sizeof(s),stdin);
  s[strcspn(s,"\n")]='\0';
  int v,k;
  cv(s,&v,&k);
  printf("Length: %d\n",strlen(s));
  printf("Vowels: %d, Consonants: %d\n",v,k);
  if(cpal(s)) {
    printf("It is a palindrome\n");
  } else {
    printf("Not a palindrome\n");
  }
  up(s);
  printf("Uppercase: %s\n",s);
  return 0;
}

void up(char s[]) {
  for(int i=0; s[i]!='\0'; ++i) {
    s[i]=toupper(s[i]);
  }
}

int cpal(char s[]) {
  char cp[100];
  int n=strlen(s);
  for(int i=0; i < n; ++i) {
    cp[i]=tolower(s[n-1-i]);
  }
  cp[n]='\0';
  for(int i=0; i<n; ++i) {
    if(tolower(s[i])!=cp[i]) {
      return 0;
    }
  }
  return 1;
}

void cv(char s[],int *v,int *k) {
  *v=0;
  *k=0;
  for(int i=0; s[i]!='\0'; ++i) {
    char c=tolower(s[i]);
    if(!isalpha(c)) {
      continue;
    }
    switch(c) {
      case 'a': case 'e': case 'i': case 'o': case 'u':
      (*v)++;
      break;
      default:
      (*k)++;
    }
  }
}
