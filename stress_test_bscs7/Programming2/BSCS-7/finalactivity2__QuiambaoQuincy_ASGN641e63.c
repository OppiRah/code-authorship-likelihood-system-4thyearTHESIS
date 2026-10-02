/*
 * Name: Quincy Quiambao
 * Program: Class Average (Structs)
 */

#include <stdio.h>

struct Student {
    char name[50];
    float grade;
};

float class_average(struct Student students[], int size)
{
    float total = 0;
    for (int i = 0; i < size; i+=1)
    {
        total += students[i].grade;
    }
    return total / size;
}

int main(void)
{
    struct Student students[3] = {{"Ana", 88}, {"Ben", 74}, {"Cara", 91}};
    printf("Class average: %.2f\n", class_average(students, 3));
    return 0;
}
