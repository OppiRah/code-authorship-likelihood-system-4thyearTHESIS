#include <stdio.h>

int main() {
    // digit sum
    int num, sum=0, t;

    printf("Enter a positive integer: ");
    scanf("%d", &num);
    t=num;
    for (; t>0; t/=10) {
        sum += t%10;
    }
    printf("\nSum of digits: %d\n", sum);
    return 0;
}
