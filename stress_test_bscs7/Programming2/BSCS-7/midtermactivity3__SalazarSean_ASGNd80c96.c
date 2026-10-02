/*
 * Name: Sean Salazar
 * Program: Reverse a String
 */

#include <stdio.h>
#include <string.h>

void reverse_string(char text[]) {
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
    reverse_string(text);
    printf("Reversed: %s\n", text);
    return 0;
}
