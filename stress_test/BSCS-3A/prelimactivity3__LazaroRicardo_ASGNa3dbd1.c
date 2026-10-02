// Practice exercise: Prime Number Check
// Submitted by: Ricardo Lazaro

#include <stdio.h>

int main() {
    // check if a number is prime
    int num = 29;
    int is_prime = 1;
    for (int i = 2; i <= num/2; i++) {
        if (num % i == 0) {
            is_prime = 0;
            break;
        }
    }
    if (is_prime) printf("%d is prime\n", num);
    else printf("%d is not prime\n", num);
    return 0;
}
