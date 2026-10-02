// Programming exercise: Inventory Value Tally
// Submitted by: Vincent Quiambao

#include <stdio.h>

struct Item {
    char name[20];
    int qty;
    float price;
};

int main()
{
	// compute total inventory value using a struct array
	struct Item item[3] = {
		{"Pen", 10, 5.5},
		{"Notebook", 4, 25.0},
		{"Eraser", 15, 3.0}
	};
	float total = 0;
	for (int i = 0; i < 3; i++) {
		total += item[i].qty * item[i].price;
	}
	printf("Total inventory value: %.2f\n", total);
	return 0;
}
