// Faith Flores - String Length

#include <stdio.h>
#include <string.h>

int getLength(char text[]) {
    int count = 0;
    while (text[count] != '\0') {
        count += 1;
    }
    return count;
}

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';
    printf("Length: %d\n", getLength(text));
    return 0;
}
