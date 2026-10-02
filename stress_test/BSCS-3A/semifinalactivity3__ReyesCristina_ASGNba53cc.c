// Practice exercise: Prime Number Check
// Submitted by: Cristina Reyes

#include <stdio.h>

int main() {
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
