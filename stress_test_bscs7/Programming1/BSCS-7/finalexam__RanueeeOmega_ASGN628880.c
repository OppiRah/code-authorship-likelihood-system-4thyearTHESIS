#include <stdio.h>
#include <string.h>

struct book {
    char title[100];
    char author[50];
    int copies;
};

void save_book(struct book b) {
    FILE *fp = fopen("library.txt", "a");
    if (fp == NULL) return;
    fprintf(fp, "%s | %s | %d copies\n", b.title, b.author, b.copies);
    fclose(fp);
}

void show_books() {
    FILE *fp = fopen("library.txt", "r");
    char line[200];
    if (fp == NULL) {
        printf("No records yet.\n");
        return;
    }
    printf("\n--- Library ---\n");
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }
    fclose(fp);
}

int main() {
    struct book b;
    printf("Enter title: ");
    scanf(" %[^\n]", b.title);
    printf("Enter author: ");
    scanf(" %[^\n]", b.author);
    printf("Enter copies: ");
    scanf("%d", &b.copies);
    save_book(b);
    show_books();
    return 0;
}
