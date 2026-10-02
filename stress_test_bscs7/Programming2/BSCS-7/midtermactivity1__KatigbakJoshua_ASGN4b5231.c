#include <stdio.h>
#include <string.h>

int get_length(char text[])
{
	int count = 0;
	while (text[count] != '\0') {
		count++;
	}
	return count;
}

int main(void)
{
	char text[100];
	printf("Enter a string: ");
	fgets(text, sizeof(text), stdin);
	text[strcspn(text, "\n")] = '\0';
	printf("Length: %d\n", get_length(text));
	return 0;
}
