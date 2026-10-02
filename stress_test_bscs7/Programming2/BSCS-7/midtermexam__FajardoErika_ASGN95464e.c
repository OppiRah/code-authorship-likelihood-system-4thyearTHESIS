// Erika Fajardo - String Analyzer

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void countVowels(char text[], int *vowels, int *consonants) {
    *vowels = 0;
    *consonants = 0;
    for (int i = 0; text[i] != '\0'; ++i) {
        char ch = tolower(text[i]);
        if (!isalpha(ch)) {
            continue;
        }
        if (strchr("aeiou", ch)) {
            (*vowels)++;
        } else {
            (*consonants)++;
        }
    }
}

int checkPalindrome(char text[]) {
    char copy[100];
    int size = strlen(text);
    for (int i = 0; i < size; ++i) {
        copy[i] = tolower(text[size - 1 - i]);
    }
    copy[size] = '\0';
    for (int i = 0; i < size; ++i) {
        if (tolower(text[i]) != copy[i]) {
            return 0;
        }
    }
    return 1;
}

void toUpper(char text[]) {
    for (int i = 0; text[i] != '\0'; ++i) {
        if (text[i] >= 'a' && text[i] <= 'z') {
            text[i] = text[i] - 32;
        }
    }
}

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';  // drop the newline
    int vowels, consonants;
    countVowels(text, &vowels, &consonants);
    printf("Length: %d\n", strlen(text));
    printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
    if (checkPalindrome(text)) {
        printf("It is a palindrome\n");
    } else {
        printf("Not a palindrome\n");
    }
    toUpper(text);
    printf("Uppercase: %s\n", text);

    if (strlen(text) > 0) {
        printf("First: %c, Last: %c\n", text[0], text[strlen(text) - 1]);
    }

    int words = 0, inWord=0;
    for (int i = 0; text[i] != '\0'; ++i) {
        if (text[i] == ' ') {
            inWord = 0;
        } else if (!inWord) {
            inWord = 1;
            ++words;
        }
    }
    printf("Words: %d\n", words);
    return 0;
}
