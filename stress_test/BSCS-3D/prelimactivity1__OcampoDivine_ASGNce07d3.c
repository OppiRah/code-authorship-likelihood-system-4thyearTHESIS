// Seatwork: Array Sum and Average
// Submitted by: Divine Ocampo

#include <stdio.h>

int main() {
    int arr[6];
    int sum = 0;
    for (int i = 0; i < 6; i++) {
        arr[i] = (i * 3 + 2) % 50;
        sum = sum + arr[i];
    }
    float avg = (float)sum / 6;
    printf("Sum = %d, Average = %.2f\n", sum, avg);
    return 0;
}
