// Practice exercise: String Analyzer (Vowels and Palindrome)
// Submitted by: Hannah Villanueva

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void analyze_string(char s[])
{
    int len = strlen(s);
    int vowels = 0, pal = 1;
    for (int i = 0; i < len; i++) {
        char c = tolower(s[i]);
        if (c=='a'||c=='e'||c=='i'||c=='o'||c=='u') vowels++;
    }
    for (int i = 0; i < len / 2; i++)
        if (s[i] != s[len-i-1]) { pal = 0; break; }
    printf("Vowels: %d\n", vowels);
    printf("Palindrome: %s\n", pal ? "yes" : "no");
}

int main()
{
    // analyze a string for vowels and palindrome status
    char word[] = "civic";
    analyze_string(word);
    return 0;
}
