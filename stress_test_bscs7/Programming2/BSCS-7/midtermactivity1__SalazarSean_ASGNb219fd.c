/*
 * Name: Sean Salazar
 * Program: String Length
 */

#include <stdio.h>
#include <string.h>

int get_length(char text[]) {
    int count=0;
    while (text[count] != '\0') {
        ++count;
    }
    return count;
}

int main() {
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';  // drop the newline
    printf("Length: %d\n", get_length(text));
    return 0;
}
