#include <stdio.h>
#include <string.h>

int main()
{
    // declare a buffer large enough to hold the input sentence
    char user_input_sentence[200];

    // ask the user to enter a full sentence
    printf("Enter a sentence: ");
    fgets(user_input_sentence, sizeof(user_input_sentence), stdin);

    // calculate the character count, excluding the trailing newline
    int total_character_count = strlen(user_input_sentence) - 1;

    // initialize the word counter and a flag for tracking word state
    int total_word_count = 0;
    int currently_inside_a_word = 0;

    // set up an index for walking through the sentence
    int scan_index = 0;

    // examine each character to detect word boundaries
    while (scan_index < total_character_count)
    {
        // check if we just entered a new word
        if (user_input_sentence[scan_index] != ' ' && !currently_inside_a_word)
        {
            currently_inside_a_word = 1;
            total_word_count = total_word_count + 1;
        }
        else if (user_input_sentence[scan_index] == ' ')
        {
            // we hit a space, so the current word has ended
            currently_inside_a_word = 0;
        }

        scan_index = scan_index + 1;
    }

    // display the final character and word counts
    printf("\nCharacter count: %d\n", total_character_count);
    printf("Word count: %d\n", total_word_count);

    return 0;
}
