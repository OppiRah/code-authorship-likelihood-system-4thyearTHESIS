/*
 * Name: Vanessa Valdez
 * Program: String Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void count_vowels(char text[], int *vowels, int *consonants)
{
    *vowels = 0;
    *consonants=0;
    for (int i = 0; text[i] != '\0'; i++)
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

void to_upper(char text[])
{
    for (int i = 0; text[i] != '\0'; i++)
    {
        text[i] = toupper(text[i]);
    }
}

int check_palindrome(char text[])
{
    int left=0, right = strlen(text) - 1;
    while (left < right)
    {
        if (tolower(text[left]) != tolower(text[right]))
        {
            return 0;
        }
        left++;
        right--;
    }
    return 1;
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
    printf("Palindrome: %s\n", check_palindrome(text) ? "yes" : "no");
    to_upper(text);
    printf("Uppercase: %s\n", text);

    if (strlen(text)>0)
    {
        printf("First: %c, Last: %c\n", text[0], text[strlen(text) - 1]);
    }

    int digit_count = 0;
    for (int i=0; text[i]!='\0'; i++)
    {
        if (isdigit(text[i]))
        {
            digit_count++;
        }
    }
    printf("Digits: %d\n", digit_count);
    return 0;
}
