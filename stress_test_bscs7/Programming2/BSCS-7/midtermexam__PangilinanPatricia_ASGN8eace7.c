// Patricia Pangilinan - String Analyzer

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void toUpper(char text[]) {
    for (int i = 0; text[i] != '\0'; ++i) {
        if (text[i] >= 'a' && text[i] <= 'z') {
            text[i] = text[i] - 32;
        }
    }
}

void countVowels(char text[], int *vowels, int *consonants) {
    *vowels = 0;
    *consonants = 0;
    for (int i = 0; text[i] != '\0'; ++i) {
        char ch = tolower(text[i]);
        if (!isalpha(ch)) {
            continue;
        }
        if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u') {
            *vowels = *vowels + 1;
        } else {
            *consonants = *consonants + 1;
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
        puts("It is a palindrome");
    } else {
        puts("Not a palindrome");
    }
    toUpper(text);
    printf("Uppercase: %s\n", text);
    int spaces = 0, index = 0;
    while (text[index] != '\0') {
        if (text[index] == ' ') {
            spaces += 1;
        }
        ++index;
    }
    printf("Spaces: %d\n", spaces);

    int words = 0, inWord = 0;
    for (int i = 0; text[i] != '\0'; ++i) {
        if (text[i] == ' ') {
            inWord = 0;
        } else if (!inWord) {
            inWord = 1;
            ++words;
        }
    }
    printf("Words: %d\n", words);

    int digitCount = 0;
    for (int i = 0; text[i] != '\0'; ++i) {
        if (isdigit(text[i])) {
            ++digitCount;
        }
    }
    printf("Digits: %d\n", digitCount);
    return 0;
}
