#include <stdio.h>
#include <string.h>

int main() {
    char strSentence[200];

    printf("Enter a sentence: ");
    fgets(strSentence, sizeof(strSentence), stdin);

    int iCharCount = strlen(strSentence) - 1;
    int iWordCount = 0, iInWord = 0, iI = 0;

    do {
        if (strSentence[iI] != ' ' && !iInWord) {
            iInWord = 1;
            iWordCount++;
        } else if (strSentence[iI] == ' ') {
            iInWord = 0;
        }
        iI++;
    } while (iI < iCharCount);

    printf("\nCharacters: %d\n", iCharCount);
    printf("Words: %d\n", iWordCount);

    return 0;
}
