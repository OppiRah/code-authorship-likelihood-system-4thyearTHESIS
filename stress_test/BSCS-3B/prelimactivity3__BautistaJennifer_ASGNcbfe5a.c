// Class activity: Prime Number Check
// Submitted by: Jennifer Bautista

#include <stdio.h>

int main() {
    // check if a number is prime
    int num = 29;
    int isPrime = 1;
    for (int i = 2; i <= num/2; i++) {
        if (num % i == 0) {
            isPrime = 0;
            break;
        }
    }
    if (isPrime) printf("%d is prime\n", num);
    else printf("%d is not prime\n", num);
    return 0;
}
