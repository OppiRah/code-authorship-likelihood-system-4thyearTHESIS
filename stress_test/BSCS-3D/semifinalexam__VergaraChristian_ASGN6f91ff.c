// Seatwork: String Analyzer (Vowels and Palindrome)
// Submitted by: Christian Vergara

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int count_vowels(char s[])
{
  int count = 0;
  for (int i = 0; s[i] != '\0'; i++) {
    char c = tolower(s[i]);
    if (c=='a'||c=='e'||c=='i'||c=='o'||c=='u') count++;
  }
  return count;
}

int is_palindrome(char s[])
{
  int len = strlen(s);
  for (int i = 0; i < len / 2; i++)
    if (s[i] != s[len - i - 1]) return 0;
  return 1;
}

int main()
{
  // count vowels and check for palindrome
  char str[] = "civic";
  printf("Vowels: %d\n", count_vowels(str));
  printf("Palindrome: %s\n", is_palindrome(str) ? "yes" : "no");
  return 0;
}
