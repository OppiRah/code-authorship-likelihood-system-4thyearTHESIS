// Practice exercise: Factorial Calculator
// Submitted by: Hannah Villanueva

#include <stdio.h>

int main()
{
    // compute factorial iteratively
    int num = 6;
    long fact = 1;
    for (int i = 1; i<=num; i++) {
        fact = fact*i;
    }
    printf("%d! = %ld\n", num, fact);
    return 0;
}
