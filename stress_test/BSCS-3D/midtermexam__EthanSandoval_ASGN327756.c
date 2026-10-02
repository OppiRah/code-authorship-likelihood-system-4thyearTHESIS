#include <stdio.h>

int main()
{
    // declare an array large enough to hold all the scores
    int array_of_student_scores[30];

    // declare the variable that holds how many scores were entered
    int total_score_count;

    // ask the user how many scores they will be entering
    printf("Enter how many scores to input: ");
    scanf("%d", &total_score_count);

    // set up a loop counter for reading the input values
    int input_index = 0;

    // read each score into the array one at a time
    while (input_index < total_score_count)
    {
        printf("Enter score %d: ", input_index + 1);
        scanf("%d", &array_of_student_scores[input_index]);
        input_index = input_index + 1;
    }

    // initialize the running total, max, and min trackers
    int running_sum_of_scores = 0;
    int highest_score_found = array_of_student_scores[0];
    int lowest_score_found = array_of_student_scores[0];

    // reset the index for the statistics computation loop
    int stats_index = 0;

    // walk through the array once to compute all the statistics
    while (stats_index < total_score_count)
    {
        // add this score to the running total
        running_sum_of_scores = running_sum_of_scores + array_of_student_scores[stats_index];

        // check if this score is higher than our current max
        if (array_of_student_scores[stats_index] > highest_score_found)
        {
            highest_score_found = array_of_student_scores[stats_index];
        }

        // check if this score is lower than our current min
        if (array_of_student_scores[stats_index] < lowest_score_found)
        {
            lowest_score_found = array_of_student_scores[stats_index];
        }

        stats_index = stats_index + 1;
    }

    // calculate the average by dividing the sum by the count
    double calculated_average_score = (double)running_sum_of_scores / total_score_count;

    // display all the computed statistics to the user
    printf("\nSum: %d\n", running_sum_of_scores);
    printf("Average: %.2lf\n", calculated_average_score);
    printf("Maximum: %d\n", highest_score_found);
    printf("Minimum: %d\n", lowest_score_found);

    return 0;
}
