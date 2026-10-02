#include <stdio.h>
#include <string.h>

// Activity 4: Character and word counter

int main() {
    char inputSentence[200];

    printf("Enter a sentence: ");
    fgets(inputSentence, sizeof(inputSentence), stdin);

    int characterCount = strlen(inputSentence) - 1; // exclude newline
    int wordCount = 0;
    int insideWord = 0;

    // Walk through the sentence and count word boundaries
    for (int i = 0; i < characterCount; i++) {
        if (inputSentence[i] != ' ' && !insideWord) {
            insideWord = 1;
            wordCount++;
        } else if (inputSentence[i] == ' ') {
            insideWord = 0;
        }
    }

    printf("\nCharacter count: %d\n", characterCount);
    printf("Word count: %d\n", wordCount);

    return 0;
}
