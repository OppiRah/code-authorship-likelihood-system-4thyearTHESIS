// Programming exercise: Temperature Converter
// Submitted by: Josefina Pascual

#include <stdio.h>

int main()
{
	// convert celsius to fahrenheit
	float cel = 25.0;
	float fah = (cel * 9 / 5) + 32;
	printf("%.1fC = %.1fF\n", cel, fah);
	return 0;
}
