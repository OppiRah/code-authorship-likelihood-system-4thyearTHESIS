#include <stdio.h>
#include <string.h>

// Final Exam: Simple library inventory system
// Combines structs and file I/O

struct Book {
    char title[100];
    char author[50];
    int copiesAvailable;
};

void saveBookToFile(struct Book book) {
    FILE *libraryFile = fopen("library.txt", "a");

    if (libraryFile == NULL) {
        printf("Error: Could not open library file.\n");
        return;
    }

    fprintf(libraryFile, "%s | %s | %d copies\n",
            book.title, book.author, book.copiesAvailable);

    fclose(libraryFile);
}

void displayAllBooks() {
    FILE *libraryFile = fopen("library.txt", "r");
    char lineBuffer[200];

    if (libraryFile == NULL) {
        printf("No records found yet.\n");
        return;
    }

    printf("\n--- Library Records ---\n");
    while (fgets(lineBuffer, sizeof(lineBuffer), libraryFile) != NULL) {
        printf("%s", lineBuffer);
    }

    fclose(libraryFile);
}

int main() {
    struct Book newBook;

    printf("Enter book title: ");
    scanf(" %[^\n]", newBook.title);

    printf("Enter author name: ");
    scanf(" %[^\n]", newBook.author);

    printf("Enter number of copies: ");
    scanf("%d", &newBook.copiesAvailable);

    saveBookToFile(newBook);
    displayAllBooks();

    return 0;
}
