#include <stdio.h>

int main() {
    FILE *fp;
    char line[100];
    fp = fopen("student_data.txt", "r");
    if (fp == NULL) {
        printf("Error opening file\n");
        return 1;
    }
    printf("--- File contents ---\n");
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }
    fclose(fp);
    return 0;
}
