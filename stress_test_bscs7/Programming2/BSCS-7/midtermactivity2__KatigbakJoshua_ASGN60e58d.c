#include <stdio.h>
#include <string.h>
#include <ctype.h>

int count_vowels(char text[])
{
	int count = 0;
	for (int i = 0; text[i] != '\0'; i++) {
		char ch = tolower(text[i]);
		if (ch=='a'||ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u') {
			count++;
		}
	}
	return count;
}

int main(void)
{
	char text[100];
	printf("Enter a string: ");
	fgets(text, sizeof(text), stdin);
	text[strcspn(text, "\n")] = '\0';
	printf("Vowels: %d\n", count_vowels(text));
	return 0;
}
