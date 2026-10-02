#include <stdio.h>
#include <string.h>
#include <ctype.h>

int gl(char s[]) {
  int c = 0;
  while(s[c]!='\0') {
    c++;
  }
  return c;
}

int cpal(char s[]) {
  char cp[100];
  int n=gl(s);
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

void up(char s[]) {
  for(int i=0; s[i]!='\0'; i++) {
    s[i]=toupper(s[i]);
  }
}

void cv(char s[],int *v,int *k) {
  *v=0;
  *k = 0;
  for(int i=0; s[i]!='\0'; i++) {
    char c2=tolower(s[i]);
    if(!isalpha(c2)) {
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

int main() {
  char s[100];
  printf("Enter a string: ");
  fgets(s,sizeof(s),stdin);
  s[strcspn(s,"\n")]='\0';
  int v,k;
  cv(s,&v,&k);
  printf("Length: %d\n",gl(s));
  printf("Vowels: %d, Consonants: %d\n",v,k);
  printf("Palindrome: %s\n",cpal(s) ? "yes" : "no");
  up(s);
  printf("\n--- Results ---\n");
  printf("Uppercase: %s\n",s);
  return 0;
}
