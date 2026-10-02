#include <stdio.h>

// this function computes the average of all scores in the array
double compute_average_of_scores(int score_array[], int number_of_scores)
{
    // initialize the running total
    int running_total = 0;

    // set up an index for walking through the array
    int sum_index = 0;

    // add up every score in the array
    while (sum_index < number_of_scores)
    {
        running_total = running_total + score_array[sum_index];
        sum_index = sum_index + 1;
    }

    // divide the total by the count to get the average
    return (double)running_total / number_of_scores;
}

// this function converts a numeric average into a letter grade
char determine_letter_grade(double numeric_average)
{
    if (numeric_average >= 90)
    {
        return 'A';
    }
    else if (numeric_average >= 80)
    {
        return 'B';
    }
    else if (numeric_average >= 70)
    {
        return 'C';
    }
    else if (numeric_average >= 60)
    {
        return 'D';
    }
    else
    {
        return 'F';
    }
}

int main()
{
    // declare an array to hold exactly five quiz scores
    int five_quiz_scores[5];

    // ask the user to enter each of the five scores
    printf("Enter 5 quiz scores:\n");
    int entry_index = 0;
    while (entry_index < 5)
    {
        printf("Score %d: ", entry_index + 1);
        scanf("%d", &five_quiz_scores[entry_index]);
        entry_index = entry_index + 1;
    }

    // compute the average and determine the corresponding grade
    double final_computed_average = compute_average_of_scores(five_quiz_scores, 5);
    char final_letter_grade = determine_letter_grade(final_computed_average);

    // display the final results to the user
    printf("\nAverage: %.2lf\n", final_computed_average);
    printf("Grade: %c\n", final_letter_grade);

    return 0;
}
