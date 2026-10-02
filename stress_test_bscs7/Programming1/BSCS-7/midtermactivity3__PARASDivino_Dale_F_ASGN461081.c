#include <stdio.h>

int main() {
    // max/min finder
    int arr[20], n;

    printf("How many numbers? ");
    scanf("%d", &n);
    for (int i=0; i<n; i++) {
        printf("Num %d: ", i+1);
        scanf("%d", &arr[i]);
    }

    int hi=arr[0], lo=arr[0];
    for (int i=1; i<n; i++) {
        if (arr[i]>hi) hi=arr[i];
        if (arr[i]<lo) lo=arr[i];
    }
    printf("\nHighest: %d\n", hi);
    printf("Lowest: %d\n", lo);
    return 0;
}
