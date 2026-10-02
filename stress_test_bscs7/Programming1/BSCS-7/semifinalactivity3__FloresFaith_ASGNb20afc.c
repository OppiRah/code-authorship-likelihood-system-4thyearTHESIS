// Faith Flores - Factorial

#include <stdio.h>

int main() {
    int num;
    long long factorial=1;
    printf("Enter a number: ");
    scanf("%d", &num);

    if (num<0) {
        printf("No factorial for negatives\n");
    } else {
        for (int i = 2; i <= num; i += 1) {
            factorial = factorial * i;  // multiply up
        }
        printf("%d! = %lld\n", num, factorial);
    }
    return 0;
}
