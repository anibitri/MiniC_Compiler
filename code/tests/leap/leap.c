// mini-c program to wether an year is leap year or not
extern int print_int(int X);

int is_leap(int year) {
    int isLeap;
    int temp;
    if (year % 4 == 0) {
        if (year % 100 == 0) {
            if (year % 400 == 0) {
                isLeap = 1;
            } else {
                isLeap = 0;
            }
        } else {
            isLeap = 1;
        }
    } else {
        isLeap = 0;
    }
    return isLeap;
}
