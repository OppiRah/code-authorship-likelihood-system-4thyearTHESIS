#include <stdio.h>

int main() {
    // bubble sort
    int arr[20], n;

    printf("How many to sort? ");
    scanf("%d", &n);
    for (int i=0; i<n; i++) {
        printf("Num %d: ", i+1);
        scanf("%d", &arr[i]);
    }

    for (int p=0; p<n-1; p++) {
        for (int j=0; j<n-p-1; j++) {
            if (arr[j]>arr[j+1]) {
                int tmp=arr[j];
                arr[j]=arr[j+1];
                arr[j+1]=tmp;
            }
        }
    }
    printf("\nSorted: ");
    for (int i=0; i<n; i++) printf("%d ", arr[i]);
    printf("\n");
    return 0;
}
