// Faith Flores - String Analyzer

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int getLength(char text[]) {
    int count = 0;
    while (text[count] != '\0') {
        count += 1;
    }
    return count;
}

int checkPalindrome(char text[]) {
    char copy[100];
    int size = getLength(text);
    for (int i = 0; i<size; i+=1) {
        copy[i] = tolower(text[size - 1 - i]);
    }
    copy[size] = '\0';
    for (int i = 0; i < size; i += 1) {
        if (tolower(text[i]) != copy[i]) {
            return 0;
        }
    }
    return 1;
}

void toUpper(char text[]) {
    for (int i = 0; text[i] != '\0'; i += 1) {
        text[i] = toupper(text[i]);
    }
}

void countVowels(char text[], int *vowels, int *consonants) {
    *vowels = 0;
    *consonants = 0;
    for (int i=0; text[i] != '\0'; i += 1) {
        char ch = tolower(text[i]);
        if (!isalpha(ch)) {
            continue;
        }
        if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u') {
            *vowels = *vowels + 1;
        } else {
            *consonants = *consonants+1;
        }
    }
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
    if (checkPalindrome(text)) {
        printf("It is a palindrome\n");
    } else {
        printf("Not a palindrome\n");
    }
    toUpper(text);
    printf("Uppercase: %s\n", text);

    int spaces = 0, index = 0;
    while (text[index] != '\0') {
        if (text[index] == ' ') {
            spaces = spaces + 1;
        }
        index += 1;
    }
    printf("Spaces: %d\n", spaces);
    int digitCount = 0;
    for (int i = 0; text[i] != '\0'; i += 1) {
        if (isdigit(text[i])) {
            digitCount += 1;
        }
    }
    printf("Digits: %d\n", digitCount);

    if (strlen(text) > 0) {
        printf("First: %c, Last: %c\n", text[0], text[strlen(text)-1]);
    }
    return 0;
}
