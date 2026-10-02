/*
 * Name: Sean Salazar
 * Program: String Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void count_vowels(char text[], int *vowels, int *consonants);
void to_upper(char text[]);
int check_palindrome(char text[]);

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';  // drop the newline
    int vowels, consonants;
    count_vowels(text, &vowels, &consonants);
    printf("Length: %d\n", strlen(text));
    printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
    if (check_palindrome(text)) {
        puts("It is a palindrome");
    } else {
        puts("Not a palindrome");
    }
    to_upper(text);
    printf("Uppercase: %s\n", text);

    int spaces = 0, index = 0;
    while (text[index] != '\0') {
        if (text[index] == ' ') {
            spaces += 1;
        }
        ++index;
    }
    printf("Spaces: %d\n", spaces);

    if (strlen(text) > 0) {
        printf("First: %c, Last: %c\n", text[0], text[strlen(text) - 1]);
    }
    return 0;
}

void count_vowels(char text[], int *vowels, int *consonants) {
    *vowels = 0;
    *consonants = 0;
    for (int i = 0; text[i] != '\0'; ++i) {
        char ch = tolower(text[i]);
        if (!isalpha(ch)) {
            continue;
        }
        if (ch == 'a' || ch == 'e' || ch == 'i'||ch == 'o' || ch == 'u') {
            *vowels = *vowels + 1;
        } else {
            *consonants = *consonants + 1;
        }
    }
}

void to_upper(char text[]) {
    for (int i = 0; text[i] != '\0'; ++i) {
        if (text[i] >= 'a' && text[i] <= 'z') {
            text[i] = text[i] - 32;
        }
    }
}

int check_palindrome(char text[]) {
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
