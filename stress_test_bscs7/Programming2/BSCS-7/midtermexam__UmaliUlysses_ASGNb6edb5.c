#include <stdio.h>
#include <string.h>
#include <ctype.h>

void up(char s[]);
void cv(char s[],int *v,int *k);
int cpal(char s[]);

int main() {
  char s[100];
  printf("Enter a string: ");
  fgets(s,sizeof(s),stdin);
  s[strcspn(s,"\n")]='\0';
  int v,k;
  cv(s,&v,&k);
  printf("Length: %d\n",strlen(s));
  printf("Vowels: %d, Consonants: %d\n",v,k);
  printf("Palindrome: %s\n",cpal(s) ? "yes" : "no");
  up(s);
  printf("Uppercase: %s\n",s);
  int s2=0,idx=0;
  while(s[idx]!='\0') {
    if(s[idx]==' ') {
      s2+=1;
    }
    idx++;
  }
  printf("Spaces: %d\n",s2);
  return 0;
}

void up(char s[]) {
  for(int i=0; s[i]!='\0'; i++) {
    s[i]=toupper(s[i]);
  }
}

void cv(char s[],int *v,int *k) {
  *v=0;
  *k = 0;
  for(int i=0; s[i]!='\0'; i++) {
    char c=tolower(s[i]);
    if(!isalpha(c)) {
      continue;
    }
    if(strchr("aeiou",c)) {
      (*v)++;
    } else {
      (*k)++;
    }
  }
}

int cpal(char s[]) {
  char cp[100];
  int n=strlen(s);
  for(int i=0; i<n; i++) {
    cp[i]=tolower(s[n-1-i]);
  }
  cp[n]='\0';
  for(int i=0; i<n; i++) {
    if(tolower(s[i])!=cp[i]) {
      return 0;
    }
  }
  return 1;
}
