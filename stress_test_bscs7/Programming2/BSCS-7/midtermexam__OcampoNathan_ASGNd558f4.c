#include <stdio.h>
#include <string.h>
#include <ctype.h>

void cv(char s[],int *v,int *k) {
  *v=0;
  *k=0;
  for(int i=0; s[i]!='\0'; i+=1) {
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

int cpal(char s[]) {
  int l=0,r=strlen(s)-1;
  while(l<r) {
    if(tolower(s[l])!=tolower(s[r])) {
      return 0;
    }
    l+=1;
    r--;
  }
  return 1;
}

void up(char s[]) {
  for(int i=0; s[i]!='\0'; i+=1) {
    s[i]=toupper(s[i]);
  }
}

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
  if(strlen(s) > 0) {
    printf("First: %c, Last: %c\n",s[0],s[strlen(s)-1]);
  }
  return 0;
}
