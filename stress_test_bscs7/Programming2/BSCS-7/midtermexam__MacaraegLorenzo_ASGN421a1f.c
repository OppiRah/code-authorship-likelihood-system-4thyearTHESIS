#include <stdio.h>
#include <string.h>
#include <ctype.h>

int gl(char s[]) {
  int c=0;
  while(s[c]!='\0') {
    c+=1;
  }
  return c;
}

void up(char s[]) {
  for(int i=0; s[i]!='\0'; i+=1) {
    if(s[i]>='a'&&s[i]<='z') {
      s[i]=s[i]-32;
    }
  }
}

void cv(char s[],int *v,int *k) {
  *v=0;
  *k=0;
  for(int i=0; s[i]!='\0'; i+=1) {
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

int cpal(char s[]) {
  int l=0,r=gl(s)-1;
  while(l<r) {
    if(tolower(s[l])!=tolower(s[r])) {
      return 0;
    }
    l+=1;
    r--;
  }
  return 1;
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
  printf("Uppercase: %s\n",s);
  int dc=0;
  for(int i=0; s[i]!='\0'; i+=1) {
    if(isdigit(s[i])) {
      dc+=1;
    }
  }
  printf("\n--- Results ---\n");
  printf("Digits: %d\n",dc);
  return 0;
}
