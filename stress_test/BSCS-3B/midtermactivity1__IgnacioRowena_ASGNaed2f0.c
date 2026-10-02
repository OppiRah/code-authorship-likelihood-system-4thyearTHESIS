// Programming exercise: String Reversal
// Submitted by: Rowena Ignacio

#include <stdio.h>
#include <string.h>

int main() {
  char str[] = "programming";
  int len = strlen(str);
  char rev[50];
  for (int i = 0; i<len; i++) {
    rev[i] = str[len - i - 1];
  }
  rev[len] = '\0';
  printf("Reversed: %s\n", rev);
  return 0;
}
