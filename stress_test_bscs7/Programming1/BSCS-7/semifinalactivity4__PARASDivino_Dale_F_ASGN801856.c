#include <stdio.h>
#include <string.h>

int main() {
    // word/char counter
    char sent[200];

    printf("Enter a sentence: ");
    fgets(sent, sizeof(sent), stdin);

    int chCt=strlen(sent)-1, wCt=0, inW=0;
    for (int i=0; i<chCt; i++) {
        if (sent[i]!=' ' && !inW) { inW=1; wCt++; }
        else if (sent[i]==' ') inW=0;
    }
    printf("\nCharacters: %d\n", chCt);
    printf("Words: %d\n", wCt);
    return 0;
}
