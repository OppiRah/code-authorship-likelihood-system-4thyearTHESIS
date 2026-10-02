/*
 * Name: Quincy Quiambao
 * Program: String Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int get_length(char text[])
{
    int count = 0;
    while (text[count] != '\0')
    {
        count += 1;
    }
    return count;
}

int check_palindrome(char text[])
{
    char copy[100];
    int size = get_length(text);
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
        switch (ch)
        {
            case 'a': case 'e': case 'i': case 'o': case 'u':
            (*vowels)++;
            break;
            default:
            (*consonants)++;
        }
    }
}

void to_upper(char text[])
{
    for (int i = 0; text[i] != '\0'; i += 1)
    {
        text[i] = toupper(text[i]);
    }
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
    printf("Length: %d\n", get_length(text));
    printf("Vowels: %d, Consonants: %d\n", vowels, consonants);
    if (check_palindrome(text))
    {
        puts("It is a palindrome");
    }
    else
    {
        puts("Not a palindrome");
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
        index += 1;
    }
    printf("Spaces: %d\n", spaces);

    int digit_count = 0;
    for (int i = 0; text[i]!='\0'; i += 1)
    {
        if (isdigit(text[i]))
        {
            digit_count += 1;
        }
    }
    printf("\n--- Results ---\n");
    printf("Digits: %d\n", digit_count);
    return 0;
}
