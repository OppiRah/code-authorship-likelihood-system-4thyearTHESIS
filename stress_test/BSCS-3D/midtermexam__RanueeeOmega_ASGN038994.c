#include <stdio.h>

int main() {
    int arr[30], n;
    printf("How many scores? ");
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        printf("Score %d: ", i+1);
        scanf("%d", &arr[i]);
    }
    int sum = 0, max = arr[0], min = arr[0];
    for (int i = 0; i < n; i++) {
        sum += arr[i];
        if (arr[i] > max) max = arr[i];
        if (arr[i] < min) min = arr[i];
    }
    double avg = (double)sum/n;
    printf("\nSum: %d\n", sum);
    printf("Avg: %.2lf\n", avg);
    printf("Max: %d\n", max);
    printf("Min: %d\n", min);
    return 0;
}
