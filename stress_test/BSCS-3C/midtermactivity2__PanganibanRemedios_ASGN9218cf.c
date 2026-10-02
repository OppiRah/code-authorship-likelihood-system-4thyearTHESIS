// Practice exercise: Linear Search
// Submitted by: Remedios Panganiban

#include <stdio.h>

int main() {
    // linear search for a target value
    int arr[8] = { 5, 17, 9, 27, 41, 3, 22, 14 };
    int target = 27;
    int idx = -1;
    for (int i = 0; i<8; i++) {
        if (arr[i] == target) {
            idx = i;
            break;
        }
    }
    if (idx != -1) printf("Found at index %d\n", idx);
    else printf("Not found\n");
    return 0;
}
