#include <stdio.h>

double avg_score(int scores[], int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += scores[i];
    return (double)sum/n;
}

char get_grade(double avg) {
    if (avg >= 90) return 'A';
    else if (avg >= 80) return 'B';
    else if (avg >= 70) return 'C';
    else if (avg >= 60) return 'D';
    else return 'F';
}

int main() {
    int scores[5];
    printf("Enter 5 quiz scores:\n");
    for (int i = 0; i < 5; i++) {
        printf("Score %d: ", i+1);
        scanf("%d", &scores[i]);
    }
    double avg = avg_score(scores, 5);
    char grade = get_grade(avg);
    printf("\nAverage: %.2lf\n", avg);
    printf("Grade: %c\n", grade);
    return 0;
}
