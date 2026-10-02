#include <stdio.h>

// grading system
double getAvg(int scores[], int n) {
    int sum=0;
    for (int i=0; i<n; i++) sum += scores[i];
    return (double)sum/n;
}

char getGrade(double avg) {
    if (avg>=90) return 'A';
    else if (avg>=80) return 'B';
    else if (avg>=70) return 'C';
    else if (avg>=60) return 'D';
    else return 'F';
}

int main() {
    int scores[5];

    printf("Enter 5 quiz scores:\n");
    for (int i=0; i<5; i++) {
        printf("Score %d: ", i+1);
        scanf("%d", &scores[i]);
    }
    double avg=getAvg(scores,5);
    printf("\nAverage: %.2lf\n", avg);
    printf("Grade: %c\n", getGrade(avg));
    return 0;
}
