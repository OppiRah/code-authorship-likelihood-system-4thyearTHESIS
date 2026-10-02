#include <stdio.h>

int main() {
    int arr[20], n;
    printf("How many to sort? ");
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        printf("Num %d: ", i+1);
        scanf("%d", &arr[i]);
    }
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (arr[j] > arr[j+1]) {
                int t = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = t;
            }
        }
    }
    printf("\nSorted: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");
    return 0;
}
