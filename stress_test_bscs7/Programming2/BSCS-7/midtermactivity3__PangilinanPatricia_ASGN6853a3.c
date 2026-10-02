// Patricia Pangilinan - Reverse a String

#include <stdio.h>
#include <string.h>

void reverseString(char text[]) {
    int left = 0, right = strlen(text) - 1;
    while (left < right) {
        char temp = text[left];
        text[left] = text[right];
        text[right] = temp;
        ++left;
        right--;
    }
}

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';  // drop the newline
    reverseString(text);
    printf("Reversed: %s\n", text);
    return 0;
}
