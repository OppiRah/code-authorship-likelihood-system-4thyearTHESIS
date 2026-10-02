/*
 * Name: Daniel Estrada
 * Program: String Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int check_palindrome(char text[])
{
    int left = 0, right = strlen(text)-1;
    while (left < right)
    {
        if (tolower(text[left]) != tolower(text[right]))
        {
            return 0;
        }
        left += 1;
        right--;
    }
    return 1;
}

void to_upper(char text[])
{
    for (int i = 0; text[i] != '\0'; i += 1)
    {
        text[i] = toupper(text[i]);
    }
}

void count_vowels(char text[], int *vowels, int *consonants)
{
    *vowels = 0;
    *consonants = 0;
    for (int i = 0; text[i] != '\0'; i += 1)
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

int main(void)
{
    char text[100];
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';
    int vowels, consonants;
    count_vowels(text, &vowels, &consonants);
    printf("Length: %d\n", strlen(text));
    printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
    printf("Palindrome: %s\n", check_palindrome(text) ? "yes" : "no");
    to_upper(text);
    printf("Uppercase: %s\n", text);

    int words = 0, in_word = 0;
    for (int i = 0; text[i] != '\0'; i += 1)
    {
        if (text[i] == ' ')
        {
            in_word = 0;
        }
        else if (!in_word)
        {
            in_word = 1;
            words += 1;
        }
    }
    printf("Words: %d\n", words);
    return 0;
}
