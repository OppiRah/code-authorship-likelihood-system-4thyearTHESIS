/*
 * Name: Mikaela Navarro
 * Program: String Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int get_length(char text[]) {
    int count = 0;
    while (text[count] != '\0') {
        count++;
    }
    return count;
}

void count_vowels(char text[], int *vowels, int *consonants) {
    *vowels = 0;
    *consonants = 0;
    for (int i = 0; text[i] != '\0'; i++) {
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

int check_palindrome(char text[]) {
    char copy[100];
    int size=get_length(text);
    for (int i = 0; i < size; i++) {
        copy[i] = tolower(text[size - 1 - i]);
    }
    copy[size]='\0';
    for (int i=0; i < size; i++) {
        if (tolower(text[i]) != copy[i]) {
            return 0;
        }
    }
    return 1;
}

void to_upper(char text[]) {
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] >= 'a' && text[i] <= 'z') {
            text[i] = text[i] - 32;
        }
    }
}

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")]='\0';  // drop the newline
    int vowels, consonants;
    count_vowels(text, &vowels, &consonants);
    printf("Length: %d\n", get_length(text));
    printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
    if (check_palindrome(text)) {
        printf("It is a palindrome\n");
    } else {
        printf("Not a palindrome\n");
    }
    to_upper(text);
    printf("Uppercase: %s\n", text);
    if (strlen(text) > 0) {
        printf("First: %c, Last: %c\n", text[0], text[strlen(text) - 1]);
    }
    int digit_count = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (isdigit(text[i])) {
            digit_count++;
        }
    }
    printf("Digits: %d\n", digit_count);
    return 0;
}
