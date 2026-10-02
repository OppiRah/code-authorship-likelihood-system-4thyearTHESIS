// Programming exercise: String Analyzer (Vowels and Palindrome)
// Submitted by: Rafael Aquino

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int cv(char s[]) {
	int count = 0;
	for (int i = 0; s[i] != '\0'; i++) {
		char c = tolower(s[i]);
		if (c=='a'||c=='e'||c=='i'||c=='o'||c=='u') count++;
	}
	return count;
}

int ip(char s[]) {
	int len = strlen(s);
	for (int i = 0; i < len / 2; i++)
		if (s[i] != s[len - i - 1]) return 0;
	return 1;
}

int main() {
	// count vowels and check for palindrome
	char str[] = "civic";
	printf("Vowels: %d\n", cv(str));
	printf("Palindrome: %s\n", ip(str) ? "yes" : "no");
	return 0;
}
