#include <stdio.h>

int main() {
    FILE *fp;
    char name[50];
    int age;
    fp = fopen("student_data.txt", "w");
    if (fp == NULL) {
        printf("Error opening file\n");
        return 1;
    }
    printf("Enter name: ");
    scanf("%s", name);
    printf("Enter age: ");
    scanf("%d", &age);
    fprintf(fp, "Name: %s\n", name);
    fprintf(fp, "Age: %d\n", age);
    fclose(fp);
    printf("\nSaved to file.\n");
    return 0;
}
