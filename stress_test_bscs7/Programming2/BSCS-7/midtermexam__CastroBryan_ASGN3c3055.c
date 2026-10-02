/*
 * Name: Bryan Castro
 * Program: String Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void count_vowels(char text[], int *vowels, int *consonants)
{
    *vowels = 0;
    *consonants = 0;
    for (int i = 0; text[i] != '\0'; ++i)
    {
        char ch = tolower(text[i]);
        if (!isalpha(ch))
        {
            continue;
        }
        if (strchr("aeiou", ch))
        {
            (*vowels)++;
        }
        else
        {
            (*consonants)++;
        }
    }
}

int check_palindrome(char text[])
{
    int left = 0, right = strlen(text) - 1;
    while (left < right)
    {
        if (tolower(text[left]) != tolower(text[right]))
        {
            return 0;
        }
        ++left;
        right--;
    }
    return 1;
}

void to_upper(char text[])
{
    for (int i = 0; text[i] != '\0'; ++i)
    {
        if (text[i] >= 'a' && text[i] <= 'z')
        {
            text[i] = text[i] - 32;
        }
    }
}

int main(void)
{
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';
    int vowels, consonants;
    count_vowels(text, &vowels, &consonants);
    printf("Length: %d\n", strlen(text));
    printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
    if (check_palindrome(text))
    {
        printf("It is a palindrome\n");
    }
    else
    {
        printf("Not a palindrome\n");
    }
    to_upper(text);
    printf("Uppercase: %s\n", text);

    int spaces = 0, index = 0;
    while (text[index] != '\0')
    {
        if (text[index] == ' ')
        {
            spaces += 1;
        }
        ++index;
    }
    printf("Spaces: %d\n", spaces);
    if (strlen(text) > 0)
    {
        printf("First: %c, Last: %c\n", text[0], text[strlen(text) - 1]);
    }
    return 0;
}
