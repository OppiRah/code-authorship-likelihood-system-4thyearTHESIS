// Isabel Jimenez - Linear Search

#include <stdio.h>

int linearSearch(int numbers[], int size, int key) {
	for (int i = 0; i < size; ++i) {
		if (numbers[i] == key) {
			return i;
		}
	}
	return -1;
}

int main() {
	int numbers[] = {5, 17, 3, 42, 8, 29};
	int key;
	printf("Search for: ");
	scanf("%d", &key);
	int index = linearSearch(numbers, 6, key);
	if (index>=0) {
		printf("Found at %d\n", index);
	}
	else {
		puts("Not found");
	}
	return 0;
}
