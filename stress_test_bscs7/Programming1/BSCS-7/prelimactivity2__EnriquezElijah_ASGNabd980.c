#include <stdio.h>

int main() {
  float c, f;
  int ch;

  printf("1 - Celsius to Fahrenheit\n2 - Fahrenheit to Celsius\n");
  scanf("%d", &ch);
  if(ch == 1)
  {
    scanf("%f", &c);
    f = c * 9 / 5+32;
    printf("%.2f F\n", f);
  } else if(ch==2) {
    scanf("%f", &f);
    c = (f - 32) * 5/9;
    printf("%.2f C\n", c);
  }
  else
  {
    printf("Invalid choice\n");
  }
  return 0;
}
