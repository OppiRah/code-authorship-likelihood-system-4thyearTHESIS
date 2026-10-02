#include <stdio.h>
#include <string.h>
#include <ctype.h>

void count_vowels(char text[], int *vowels, int *consonants)
{
	*vowels = 0;
	*consonants = 0;
	for (int i = 0; text[i] != '\0'; i++) {
		char ch = tolower(text[i]);
		if (!isalpha(ch)) {
			continue;
		}
		if (ch == 'a' || ch == 'e' || ch=='i' || ch == 'o' || ch == 'u') {
			*vowels = *vowels+1;
		} else {
			*consonants = *consonants + 1;
		}
	}
}

void to_upper(char text[])
{
	for (int i = 0; text[i] != '\0'; i++) {
		if (text[i] >= 'a' && text[i] <= 'z') {
			text[i] = text[i] - 32;
		}
	}
}

int check_palindrome(char text[])
{
	char copy[100];
	int size = strlen(text);
	for (int i = 0; i < size; i++) {
		copy[i] = tolower(text[size - 1 - i]);
	}
	copy[size] = '\0';
	for (int i = 0; i < size; i++) {
		if (tolower(text[i]) != copy[i]) {
			return 0;
		}
	}
	return 1;
}

int main(void)
{
	char text[100];
	fgets(text, sizeof(text), stdin);
	// drop the newline
	text[strcspn(text, "\n")] = '\0';
	int vowels, consonants;
	count_vowels(text, &vowels, &consonants);
	printf("Length: %d\n", strlen(text));
	printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
	printf("Palindrome: %s\n", check_palindrome(text) ? "yes" : "no");
	to_upper(text);
	printf("Uppercase: %s\n", text);

	int digit_count = 0;
	for (int i = 0; text[i] != '\0'; i++) {
		if (isdigit(text[i])) {
			digit_count++;
		}
	}
	printf("Digits: %d\n", digit_count);
	return 0;
}
