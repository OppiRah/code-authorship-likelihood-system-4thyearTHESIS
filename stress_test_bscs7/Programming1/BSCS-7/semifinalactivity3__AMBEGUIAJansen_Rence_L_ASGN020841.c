#include <stdio.h>
#include <string.h>

int IsPalindrome(char strS[]) {
    int iLen = strlen(strS);
    int iI = 0;

    do {
        if (strS[iI] != strS[iLen - 1 - iI]) {
            return 0;
        }
        iI++;
    } while (iI < iLen / 2);

    return 1;
}

int main() {
    char strWord[100];

    printf("Enter a word: ");
    scanf("%s", strWord);

    if (IsPalindrome(strWord)) {
        printf("\n%s is a palindrome.\n", strWord);
    } else {
        printf("\n%s is not a palindrome.\n", strWord);
    }

    return 0;
}
