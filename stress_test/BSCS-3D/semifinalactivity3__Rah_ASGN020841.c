#include <stdio.h>
#include <string.h>

// Activity 3: Palindrome string checker

int isPalindrome(char text[]) {
    int length = strlen(text);

    for (int i = 0; i < length / 2; i++) {
        if (text[i] != text[length - 1 - i]) {
            return 0;
        }
    }

    return 1;
}

int main() {
    char inputText[100];

    printf("Enter a word or phrase (no spaces): ");
    scanf("%s", inputText);

    if (isPalindrome(inputText)) {
        printf("\n\"%s\" is a palindrome.\n", inputText);
    } else {
        printf("\n\"%s\" is not a palindrome.\n", inputText);
    }

    return 0;
}
