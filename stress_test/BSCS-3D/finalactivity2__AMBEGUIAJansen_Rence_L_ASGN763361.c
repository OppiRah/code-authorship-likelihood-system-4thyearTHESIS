// Java version
import java.util.Scanner;

public class Activity2_Jansen {
    static void swapVals(int[] values) {
        int temp = values[0];
        values[0] = values[1];
        values[1] = temp;
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        System.out.print("Enter first value: ");
        int x = scanner.nextInt();
        System.out.print("Enter second value: ");
        int y = scanner.nextInt();

        System.out.printf("%nBefore: %d, %d%n", x, y);
        int[] values = {x, y};
        swapVals(values);
        System.out.printf("After: %d, %d%n", values[0], values[1]);
        scanner.close();
    }
}
