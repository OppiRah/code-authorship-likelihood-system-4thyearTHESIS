#include <stdio.h>
#include <string.h>

int main() {
    char sentence[200];
    printf("Enter a sentence: ");
    fgets(sentence, sizeof(sentence), stdin);
    int char_ct = strlen(sentence)-1;
    int word_ct = 0, in_word = 0;
    for (int i = 0; i < char_ct; i++) {
        if (sentence[i] != ' ' && !in_word) {
            in_word = 1;
            word_ct++;
        } else if (sentence[i] == ' ') {
            in_word = 0;
        }
    }
    printf("\nChars: %d\n", char_ct);
    printf("Words: %d\n", word_ct);
    return 0;
}
