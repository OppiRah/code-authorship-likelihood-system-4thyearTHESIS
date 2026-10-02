// Aaron Aquino - Count Vowels

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int countVowels(char text[]) {
    int count = 0;
    for (int i = 0; text[i] != '\0'; ++i) {
        char ch = tolower(text[i]);
        if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u') {
            ++count;
        }
    }
    return count;
}

int main() {
    char text[100];
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';
    printf("Vowels: %d\n", countVowels(text));
    return 0;
}
