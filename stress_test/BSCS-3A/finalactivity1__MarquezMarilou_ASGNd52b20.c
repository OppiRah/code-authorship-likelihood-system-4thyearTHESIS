// Seatwork: String Reversal
// Submitted by: Marilou Marquez

#include <stdio.h>
#include <string.h>

int main() {
    // reverse a string manually
    char str[] = "programming";
    int len = strlen(str);
    char rev[50];
    for (int i = 0; i<len; i++) {
        rev[i] = str[len - i - 1];
    }
    rev[len] = '\0';
    printf("Reversed: %s\n", rev);
    return 0;
}
