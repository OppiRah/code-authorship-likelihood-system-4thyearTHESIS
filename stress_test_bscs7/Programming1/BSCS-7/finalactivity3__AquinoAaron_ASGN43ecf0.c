// Aaron Aquino - Count Even and Odd

#include <stdio.h>

int main() {
    int numbers[50], size;
    scanf("%d", &size);
    for (int i = 0; i < size; ++i) {
        scanf("%d", &numbers[i]);
    }
    int evenCount = 0, oddCount = 0;
    for (int i = 0; i < size; ++i) {
        if (numbers[i] % 2 == 0) {
            ++evenCount;
        } else {
            ++oddCount;
        }
    }
    printf("Even: %d, Odd: %d\n", evenCount, oddCount);
    return 0;
}
