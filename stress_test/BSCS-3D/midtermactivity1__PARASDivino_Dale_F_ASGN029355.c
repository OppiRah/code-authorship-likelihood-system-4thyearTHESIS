#include <stdio.h>

int main() {
    // mult table
    int num;

    printf("Enter number: ");
    scanf("%d", &num);
    printf("\nMultiplication table:\n");
    for (int i=1; i<=10; i++) {
        printf("%d x %d = %d\n", num, i, num*i);
    }
    return 0;
}
