#include <stdio.h>
#include <string.h>

int is_palindrome(char s[]) {
    int len = strlen(s);
    for (int i = 0; i < len/2; i++) {
        if (s[i] != s[len-1-i]) return 0;
    }
    return 1;
}

int main() {
    char word[100];
    printf("Enter a word: ");
    scanf("%s", word);
    if (is_palindrome(word)) printf("\n%s is a palindrome.\n", word);
    else printf("\n%s is not a palindrome.\n", word);
    return 0;
}
