#include <stdio.h>
#include <string.h>

// palindrome checker
int isPalin(char s[]) {
    int len=strlen(s);
    for (int i=0; i<len/2; i++) {
        if (s[i]!=s[len-1-i]) return 0;
    }
    return 1;
}

int main() {
    char w[100];

    printf("Enter a word: ");
    scanf("%s", w);
    printf(isPalin(w) ? "\n%s is a palindrome.\n" : "\n%s is not a palindrome.\n", w);
    return 0;
}
