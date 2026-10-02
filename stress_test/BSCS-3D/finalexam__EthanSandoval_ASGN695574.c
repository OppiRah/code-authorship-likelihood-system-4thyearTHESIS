#include <stdio.h>
#include <string.h>

// this structure holds all the information about a single book
struct Book
{
    char book_title[100];
    char book_author[50];
    int number_of_copies_available;
};

// this function appends a single book record to the library file
void save_book_record_to_file(struct Book book_to_save)
{
    // attempt to open the library file in append mode
    FILE *library_file_pointer = fopen("library.txt", "a");

    // check whether the file was opened successfully
    if (library_file_pointer == NULL)
    {
        printf("Error: Could not open library file.\n");
        return;
    }

    // write the book's details as a single formatted line
    fprintf(library_file_pointer, "%s | %s | %d copies\n",
            book_to_save.book_title, book_to_save.book_author,
            book_to_save.number_of_copies_available);

    // close the file now that we are finished writing to it
    fclose(library_file_pointer);
}

// this function reads and displays every book currently on record
void display_all_saved_books()
{
    // attempt to open the library file in read mode
    FILE *library_file_pointer = fopen("library.txt", "r");

    // declare a buffer to hold each line as it is read
    char line_reading_buffer[200];

    // check whether the file was opened successfully
    if (library_file_pointer == NULL)
    {
        printf("No records found yet.\n");
        return;
    }

    printf("\n--- Library Records ---\n");

    // read and display each line until the end of the file is reached
    while (fgets(line_reading_buffer, sizeof(line_reading_buffer), library_file_pointer) != NULL)
    {
        printf("%s", line_reading_buffer);
    }

    // close the file now that we are finished reading from it
    fclose(library_file_pointer);
}

int main()
{
    // declare a single book record to be filled in by the user
    struct Book new_book_to_add;

    // ask the user for the book's title
    printf("Enter book title: ");
    scanf(" %[^\n]", new_book_to_add.book_title);

    // ask the user for the author's name
    printf("Enter author name: ");
    scanf(" %[^\n]", new_book_to_add.book_author);

    // ask the user how many copies are available
    printf("Enter number of copies: ");
    scanf("%d", &new_book_to_add.number_of_copies_available);

    // save the new book record and then display the full library
    save_book_record_to_file(new_book_to_add);
    display_all_saved_books();

    return 0;
}
