#include <stdio.h>

int main()
{
  // compute total inventory value using parallel arrays
  char names[3][20] = { "Pen", "Notebook", "Eraser" };
  int qty[3] = { 10, 4, 15 };
  float price[3] = { 5.5, 25.0, 3.0 };
  float gt = 0;
  for (int i = 0; i < 3; i++) {
    printf("%s: %d x %.2f\n", names[i], qty[i], price[i]);
    gt += qty[i] * price[i];
  }
  printf("Total: %.2f\n", gt);
  return 0;
}
