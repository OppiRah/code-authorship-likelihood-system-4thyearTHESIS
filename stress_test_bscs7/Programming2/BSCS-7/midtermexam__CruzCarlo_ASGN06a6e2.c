// Carlo Cruz - String Analyzer

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void countVowels(char text[], int *vowels, int *consonants);
void toUpper(char text[]);
int checkPalindrome(char text[]);
int getLength(char text[]);

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';  // drop the newline
    int vowels, consonants;
    countVowels(text, &vowels, &consonants);
    printf("Length: %d\n", getLength(text));
    printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
    printf("Palindrome: %s\n", checkPalindrome(text) ? "yes" : "no");
    toUpper(text);
    printf("Uppercase: %s\n", text);

    int digitCount = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (isdigit(text[i])) {
            digitCount++;
        }
    }
    printf("Digits: %d\n", digitCount);

    int spaces = 0, index = 0;
    while (text[index] != '\0') {
        if (text[index] == ' ') {
            spaces += 1;
        }
        index++;
    }
    printf("Spaces: %d\n", spaces);
    return 0;
}

void countVowels(char text[], int *vowels, int *consonants) {
    *vowels = 0;
    *consonants = 0;
    for (int i = 0; text[i] != '\0'; i++) {
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

void toUpper(char text[]) {
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] >= 'a' && text[i] <= 'z') {
            text[i] = text[i] - 32;
        }
    }
}

int checkPalindrome(char text[]) {
    int left = 0, right = getLength(text) - 1;
    while (left < right) {
        if (tolower(text[left]) != tolower(text[right])) {
            return 0;
        }
        left++;
        right--;
    }
    return 1;
}

int getLength(char text[]) {
    int count = 0;
    while (text[count] != '\0') {
        count++;
    }
    return count;
}
