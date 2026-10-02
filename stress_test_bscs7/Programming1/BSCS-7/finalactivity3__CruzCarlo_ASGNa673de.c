// Carlo Cruz - Count Even and Odd

#include <stdio.h>

int main() {
    int numbers[50], size;
    printf("How many numbers? ");
    scanf("%d", &size);
    for (int i = 0; i < size; i++) {
        printf("Enter number %d: ", i + 1);
        scanf("%d", &numbers[i]);
    }
    int evenCount = 0, oddCount = 0;

    for (int i = 0; i < size; i++) {
        if (numbers[i] % 2 == 0) {
            evenCount++;
        } else {
            oddCount++;
        }
    }
    printf("Even: %d, Odd: %d\n", evenCount, oddCount);
    return 0;
}
