#include <stdio.h>

int main() {
    int arr[20], n;
    printf("How many numbers? ");
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        printf("Num %d: ", i+1);
        scanf("%d", &arr[i]);
    }
    int max = arr[0], min = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] > max) max = arr[i];
        if (arr[i] < min) min = arr[i];
    }
    printf("\nMax: %d\n", max);
    printf("Min: %d\n", min);
    return 0;
}
