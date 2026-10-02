#include <stdio.h>
#include <string.h>

// library system
struct Book {
    char title[100];
    char author[50];
    int copies;
};

void saveBook(struct Book b) {
    FILE *fp=fopen("library.txt", "a");
    if (fp==NULL) return;
    fprintf(fp, "%s | %s | %d copies\n", b.title, b.author, b.copies);
    fclose(fp);
}

void showBooks() {
    FILE *fp=fopen("library.txt", "r");
    char buf[200];
    if (fp==NULL) {
        printf("No records found.\n");
        return;
    }
    printf("\n--- Library Records ---\n");
    while (fgets(buf, sizeof(buf), fp)!=NULL) {
        printf("%s", buf);
    }
    fclose(fp);
}

int main() {
    struct Book nb;

    printf("Enter title: ");
    scanf(" %[^\n]", nb.title);
    printf("Enter author: ");
    scanf(" %[^\n]", nb.author);
    printf("Enter copies: ");
    scanf("%d", &nb.copies);
    saveBook(nb);
    showBooks();
    return 0;
}
