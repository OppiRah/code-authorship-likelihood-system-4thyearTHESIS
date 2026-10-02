#include <stdio.h>

// student record
struct Student {
    char nm[50];
    int ag;
    double gp;
};

int main() {
    struct Student s;

    printf("Enter name: ");
    scanf("%s", s.nm);
    printf("Enter age: ");
    scanf("%d", &s.ag);
    printf("Enter GPA: ");
    scanf("%lf", &s.gp);
    printf("\nName: %s\n", s.nm);
    printf("Age: %d\n", s.ag);
    printf("GPA: %.2lf\n", s.gp);
    return 0;
}
