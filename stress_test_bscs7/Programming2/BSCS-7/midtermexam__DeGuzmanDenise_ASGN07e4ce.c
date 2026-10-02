/*
 * Name: Denise De Guzman
 * Program: String Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void to_upper(char text[]);
int check_palindrome(char text[]);
void count_vowels(char text[], int *vowels, int *consonants);

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';  // drop the newline
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
    int spaces = 0, index = 0;
    while (text[index] != '\0') {
        if (text[index] == ' ') {
            spaces = spaces + 1;
        }
        index++;
    }
    printf("Spaces: %d\n", spaces);
    return 0;
}

void to_upper(char text[]) {
    for (int i=0; text[i] != '\0'; i++) {
        text[i] = toupper(text[i]);
    }
}

int check_palindrome(char text[]) {
    char copy[100];
    int size = strlen(text);
    for (int i = 0; i < size; i++) {
        copy[i] = tolower(text[size - 1 - i]);
    }
    copy[size]='\0';
    for (int i = 0; i < size; i++) {
        if (tolower(text[i]) != copy[i]) {
            return 0;
        }
    }
    return 1;
}

void count_vowels(char text[], int *vowels, int *consonants) {
    *vowels = 0;
    *consonants = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        char ch=tolower(text[i]);
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
