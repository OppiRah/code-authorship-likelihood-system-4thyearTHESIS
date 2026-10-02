// Isabel Jimenez - String Analyzer

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int getLength(char text[]) {
	int count = 0;
	while (text[count] != '\0') {
		++count;
	}
	return count;
}

void toUpper(char text[]) {
	for (int i = 0; text[i] != '\0'; ++i) {
		text[i] = toupper(text[i]);
	}
}

void countVowels(char text[], int *vowels, int *consonants) {
	*vowels = 0;
	*consonants=0;
	for (int i = 0; text[i] != '\0'; ++i) {
		char ch = tolower(text[i]);
		if (!isalpha(ch)) {
			continue;
		}
		if (ch == 'a'||ch == 'e' || ch == 'i' || ch=='o' || ch == 'u') {
			*vowels = *vowels + 1;
		}
		else {
			*consonants = *consonants + 1;
		}
	}
}

int checkPalindrome(char text[]) {
	int left = 0, right = getLength(text) - 1;
	while (left < right) {
		if (tolower(text[left]) != tolower(text[right])) {
			return 0;
		}
		++left;
		right--;
	}
	return 1;
}

int main() {
	char text[100];
	printf("Enter a string: ");
	fgets(text, sizeof(text), stdin);
	text[strcspn(text, "\n")] = '\0';
	int vowels, consonants;
	countVowels(text, &vowels, &consonants);
	printf("Length: %d\n", getLength(text));
	printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
	printf("Palindrome: %s\n", checkPalindrome(text) ? "yes" : "no");
	toUpper(text);
	printf("Uppercase: %s\n", text);
	int digitCount=0;
	for (int i = 0; text[i] != '\0'; ++i) {
		if (isdigit(text[i])) {
			++digitCount;
		}
	}
	printf("Digits: %d\n", digitCount);

	int words = 0, inWord = 0;
	for (int i = 0; text[i] != '\0'; ++i) {
		if (text[i]==' ') {
			inWord = 0;
		}
		else if (!inWord) {
			inWord = 1;
			++words;
		}
	}
	printf("Words: %d\n", words);
	if (strlen(text)>0) {
		printf("First: %c, Last: %c\n", text[0], text[strlen(text)-1]);
	}
	printf("\n--- Results ---\n");
	return 0;
}
