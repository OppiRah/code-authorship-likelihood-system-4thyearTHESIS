/*
 * Name: Rhea Ramirez
 * Program: String Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void to_upper(char text[])
{
    for (int i = 0; text[i] != '\0'; i += 1)
    {
        if (text[i] >= 'a' && text[i] <= 'z')
        {
            text[i] = text[i] - 32;
        }
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
        if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o'||ch == 'u')
        {
            *vowels=*vowels+1;
        }
        else
        {
            *consonants = *consonants + 1;
        }
    }
}

int check_palindrome(char text[])
{
    char copy[100];
    int size = strlen(text);
    for (int i = 0; i < size; i += 1)
    {
        copy[i] = tolower(text[size - 1 - i]);
    }
    copy[size] = '\0';
    for (int i = 0; i < size; i += 1)
    {
        if (tolower(text[i]) != copy[i])
        {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    char text[100];
    printf("Enter a string: ");
    fgets(text, sizeof(text), stdin);
    // drop the newline
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
            spaces = spaces + 1;
        }
        index += 1;
    }
    printf("Spaces: %d\n", spaces);
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

    int digit_count = 0;
    for (int i = 0; text[i] != '\0'; i += 1)
    {
        if (isdigit(text[i]))
        {
            digit_count += 1;
        }
    }
    printf("Digits: %d\n", digit_count);
}
