// Programming exercise: Temperature Converter
// Submitted by: Kristine Vergara

#include <stdio.h>

int main()
{
	// convert celsius to fahrenheit
	float celsius = 25.0;
	float fahrenheit = (celsius * 9 / 5) + 32;
	printf("%.1fC = %.1fF\n", celsius, fahrenheit);
	return 0;
}
