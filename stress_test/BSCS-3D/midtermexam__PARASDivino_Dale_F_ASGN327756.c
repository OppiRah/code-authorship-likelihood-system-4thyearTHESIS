#include <stdio.h>

int main() {
    // array stats
    int arr[30], n;

    printf("How many scores? ");
    scanf("%d", &n);
    for (int i=0; i<n; i++) {
        printf("Score %d: ", i+1);
        scanf("%d", &arr[i]);
    }

    int sum=0, hi=arr[0], lo=arr[0];
    for (int i=0; i<n; i++) {
        sum += arr[i];
        if (arr[i]>hi) hi=arr[i];
        if (arr[i]<lo) lo=arr[i];
    }
    double avg=(double)sum/n;
    printf("\nSum: %d\n", sum);
    printf("Average: %.2lf\n", avg);
    printf("Highest: %d\n", hi);
    printf("Lowest: %d\n", lo);
    return 0;
}
