#include <stdio.h>
#include <string.h>

// this function checks whether the given text reads the same backwards
int check_if_text_is_palindrome(char text_to_check[])
{
    // determine how many characters are in the text
    int text_length = strlen(text_to_check);

    // set up an index starting from the beginning of the text
    int comparison_index = 0;

    // compare characters from both ends moving towards the middle
    while (comparison_index < text_length / 2)
    {
        // if the mirrored characters do not match, it is not a palindrome
        if (text_to_check[comparison_index] != text_to_check[text_length - 1 - comparison_index])
        {
            return 0;
        }

        comparison_index = comparison_index + 1;
    }

    // all mirrored characters matched, so it is a palindrome
    return 1;
}

int main()
{
    // declare a buffer to hold the user's input text
    char user_input_text[100];

    // ask the user for a word or phrase without spaces
    printf("Enter a word or phrase (no spaces): ");
    scanf("%s", user_input_text);

    // call the palindrome-checking function and report the result
    if (check_if_text_is_palindrome(user_input_text))
    {
        printf("\n\"%s\" is a palindrome.\n", user_input_text);
    }
    else
    {
        printf("\n\"%s\" is not a palindrome.\n", user_input_text);
    }

    return 0;
}
