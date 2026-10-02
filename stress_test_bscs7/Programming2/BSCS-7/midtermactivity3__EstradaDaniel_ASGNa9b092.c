/*
 * Name: Daniel Estrada
 * Program: Reverse a String
 */

#include <stdio.h>
#include <string.h>

void reverse_string(char text[])
{
    int left = 0, right = strlen(text) - 1;
    while (left < right)
    {
        char temp = text[left];
        text[left] = text[right];
        text[right] = temp;
        left += 1;
        right--;
    }
}

int main(void)
{
    char text[100];
    fgets(text, sizeof(text), stdin);
    // drop the newline
    text[strcspn(text, "\n")] = '\0';
    reverse_string(text);
    printf("Reversed: %s\n", text);
    return 0;
}
