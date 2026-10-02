// Carlo Cruz - String Length

#include <stdio.h>
#include <string.h>

int getLength(char text[]) {
    int count = 0;
    while (text[count] != '\0') {
        count++;
    }
    return count;
}

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';  // drop the newline
    printf("Length: %d\n", getLength(text));
    return 0;
}
