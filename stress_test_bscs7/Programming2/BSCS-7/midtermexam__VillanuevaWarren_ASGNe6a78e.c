/*
 * Name: Warren Villanueva
 * Program: String Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void count_vowels(char text[], int *vowels, int *consonants);
void to_upper(char text[]);
int check_palindrome(char text[]);
int get_length(char text[]);

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';  // drop the newline
    int vowels, consonants;
    count_vowels(text, &vowels, &consonants);
    printf("Length: %d\n", get_length(text));
    printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
    printf("Palindrome: %s\n", check_palindrome(text) ? "yes" : "no");
    to_upper(text);
    printf("Uppercase: %s\n", text);
    int words = 0, in_word=0;
    for (int i = 0; text[i] != '\0'; i += 1) {
        if (text[i] == ' ') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words += 1;
        }
    }
    printf("\n--- Results ---\n");
    printf("Words: %d\n", words);
}

void count_vowels(char text[], int *vowels, int *consonants) {
    *vowels = 0;
    *consonants=0;
    for (int i = 0; text[i] != '\0'; i += 1) {
        char ch = tolower(text[i]);
        if (!isalpha(ch)) {
            continue;
        }
        switch (ch) {
            case 'a': case 'e': case 'i': case 'o': case 'u':
            (*vowels)++;
            break;
            default:
            (*consonants)++;
        }
    }
}

void to_upper(char text[]) {
    for (int i = 0; text[i] != '\0'; i += 1) {
        text[i] = toupper(text[i]);
    }
}

int check_palindrome(char text[]) {
    int left = 0, right = get_length(text)-1;
    while (left < right) {
        if (tolower(text[left]) != tolower(text[right])) {
            return 0;
        }
        left += 1;
        right--;
    }
    return 1;
}

int get_length(char text[]) {
    int count = 0;
    while (text[count] != '\0') {
        count += 1;
    }
    return count;
}
