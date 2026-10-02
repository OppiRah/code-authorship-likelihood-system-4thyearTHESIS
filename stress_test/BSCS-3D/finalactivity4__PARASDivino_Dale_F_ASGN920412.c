#include <stdio.h>

int main() {
    // read from file
    FILE *fp;
    char buf[100];

    fp=fopen("student_data.txt", "r");
    if (fp==NULL) {
        printf("Error opening file.\n");
        return 1;
    }
    printf("--- File Contents ---\n");
    while (fgets(buf, sizeof(buf), fp)!=NULL) {
        printf("%s", buf);
    }
    fclose(fp);
    return 0;
}
